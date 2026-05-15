#include "arena_client.h"

#include <common/encoding.h>
#include <common/hash.h>
#include <common/log.h>
#include <db/db_song.h>
#include <functional>
#include <game/arena/arena_data.h>
#include <game/arena/arena_internal.h>
#include <game/runtime/i18n.h>
#include <game/runtime/state.h>
#include <game/scene/scene_context.h>
#include <git_version.h>

#include <exception>
#include <set>

#include <boost/format.hpp>
#include <cereal/archives/portable_binary.hpp>

std::shared_ptr<ArenaClient> g_pArenaClient = nullptr;

ArenaClient::~ArenaClient()
{
    if (gArenaData.isOnline())
    {
        leaveLobby();
    }
    if (socket)
    {
        ioc.stop();
        listen.wait();
        socket.reset();
    }
}

static void emptyHandleSend(const std::shared_ptr<std::vector<unsigned char>>& message,
                            const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
{
    if (error)
    {
        LOG_WARNING << error.message();
    }
}

std::vector<ArenaLobbyInfo> ArenaClient::seekLobby()
{
    // TBD
    return {};
}

bool ArenaClient::joinLobby(const std::string& address)
{
    if (gArenaData.isOnline())
    {
        LOG_WARNING << "[Arena] Join failed! Please leave your current lobby first.";
        createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_IN_LOBBY));
        return false;
    }

    LOG_INFO << "[Arena] Resolving address " << address;
    udp::resolver resolver(ioc);
    try
    {
        if (auto v4addr = resolver.resolve(udp::v4(), address, ""); !v4addr.empty())
        {
            server = *v4addr.begin();
            server.port(ARENA_HOST_PORT);
        }
        else if (auto v6addr = resolver.resolve(udp::v6(), address, ""); !v6addr.empty())
        {
            server = *v6addr.begin();
            server.port(ARENA_HOST_PORT);
        }
        else
        {
            createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_ADDRESS_ERROR));
            LOG_WARNING << "[Arena] Address error";
            return false;
        }
    }
    catch (const std::exception& e)
    {
        createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_ADDRESS_ERROR));
        LOG_WARNING << "[Arena] Address error";
        return false;
    }

    LOG_INFO << "[Arena] Address: " << server.address().to_string();
    LOG_INFO << "[Arena] Port: " << server.port();

    try
    {
        if (server.address().is_v4())
        {
            socket = std::make_shared<udp::socket>(ioc);
            socket->open(udp::v4());
        }
        else if (server.address().is_v6())
        {
            socket = std::make_shared<udp::socket>(ioc);
            socket->open(udp::v6());
        }
        else
        {
            lunaticvibes::verify_failed("[Arena] ???");
            return false;
        }
    }
    catch (std::exception& e)
    {
        LOG_WARNING << "[Arena] Open socket failed: " << e.what();
        return false;
    }

    LOG_INFO << "[Arena] Joining lobby...";

    auto n = std::make_shared<ArenaMessageJoinLobby>();
    n->messageIndex = ++sendMessageIndex;
    n->version = GIT_REVISION;
    n->playerName = State::get(IndexText::PLAYER_NAME);

    auto payload = n->pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
    addTaskWaitingForResponse(n->messageIndex, payload);

    gArenaData.online = true;
    asyncRecv();
    listen = std::async(std::launch::async, [&] {
        try
        {
            ioc.run();
        }
        catch (std::exception& e)
        {
            LOG_ERROR << "[ArenaClient] Listener exception: " << e.what();
            // FIXME: stop arena
        }
    });

    // FIXME what is this code
    for (int i = 0; i < 50; ++i)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
        if (joinLobbyErrorCode > 0)
        {
            switch (static_cast<ArenaErrorCode>(joinLobbyErrorCode))
            {
            case ArenaErrorCode::NotEnoughSlots: createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_FULL)); break;
            case ArenaErrorCode::HostIsPlaying: createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_IN_GAME)); break;
            case ArenaErrorCode::VersionMismatch:
                createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_VERSION_ERROR));
                break;
            case ArenaErrorCode::DuplicateAddress:
                createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_DUPLICATE_CLIENT));
                break;
            }
            LOG_WARNING << "[ArenaClient] Join failed: [" << joinLobbyErrorCode << "] "
                        << static_cast<ArenaErrorCode>(joinLobbyErrorCode);
            return false;
        }
        if (joinLobbyErrorCode == 0)
        {
            State::set(IndexText::ARENA_LOBBY_STATUS, "ARENA LOBBY");
            return true;
        }
    }

    createNotification(i18n::s(i18nText::ARENA_JOIN_FAILED_TIMEOUT));
    LOG_WARNING << "[Arena] Join failed: Timeout 5s";
    return false;
}

void ArenaClient::leaveLobby()
{
    if (!gArenaData.isOnline())
        return;

    if (!gArenaData.isExpired())
    {
        auto n = std::make_shared<ArenaMessageLeaveLobby>();
        n->messageIndex = ++sendMessageIndex;

        auto payload = n->pack();
        socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
        // no response
    }

    ioc.stop();
    listen.wait();
    socket.reset();

    gArenaData.expired = true;
}

void ArenaClient::requestChart(const HashMD5& reqChart)
{
    requestChartHash = reqChart;
    auto n = std::make_shared<ArenaMessageRequestChart>();
    n->messageIndex = ++sendMessageIndex;
    n->chartHashMD5String = !reqChart.empty() ? reqChart.hexdigest() : "";

    auto payload = n->pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    addTaskWaitingForResponse(n->messageIndex, payload);
}

void ArenaClient::setLoadingFinished(int playStartTimeMs)
{
    if (_isLoadingFinished)
        return;

    _isLoadingFinished = true;

    auto n = std::make_shared<ArenaMessageClientFinishedLoading>();
    n->messageIndex = ++sendMessageIndex;
    n->playStartTimeMs = playStartTimeMs;

    auto payload = n->pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    addTaskWaitingForResponse(n->messageIndex, payload);
}

void ArenaClient::setCreatedRuleset()
{
    if (_isCreatedRuleset)
        return;

    _isCreatedRuleset = true;

    auto n = std::make_shared<ArenaMessageClientPlayInit>();
    n->messageIndex = ++sendMessageIndex;
    n->payload = vRulesetNetwork::packInit(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]);

    auto payload = n->pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    addTaskWaitingForResponse(n->messageIndex, payload);
}

void ArenaClient::setPlayingFinished()
{
    if (_isPlayingFinished)
        return;

    _isPlayingFinished = true;

    auto n = std::make_shared<ArenaMessageClientFinishedPlaying>();
    n->messageIndex = ++sendMessageIndex;

    auto payload = n->pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    addTaskWaitingForResponse(n->messageIndex, payload);
}

void ArenaClient::setResultFinished()
{
    if (_isResultFinished)
        return;

    _isResultFinished = true;

    auto n = std::make_shared<ArenaMessageClientFinishedResult>();
    n->messageIndex = ++sendMessageIndex;

    auto payload = n->pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    addTaskWaitingForResponse(n->messageIndex, payload);
}

void ArenaClient::asyncRecv()
{
    socket->async_receive_from(boost::asio::buffer(recv_buf), remote_endpoint,
                               std::bind_front(&ArenaClient::handleRecv, this));
}

void ArenaClient::handleRecv(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (error)
    {
        LOG_WARNING << "[Arena] socket exception: " << error.message();

        ioc.stop();
        listen.wait();
        socket.reset();

        gArenaData.expired = true;
        return;
    }

    if (bytes_transferred > recv_buf.size())
    {
        LOG_DEBUG << "[Arena] Ignored huge packet (" << bytes_transferred << ")";
        return;
    }
    handleRequest(recv_buf.data(), bytes_transferred);

    if (gArenaData.isOnline() && !gArenaData.isExpired())
        asyncRecv();
}

void ArenaClient::handleRequest(const unsigned char* recv_buf, size_t recv_buf_len)
{
    auto pMsg = ArenaMessage::unpack(recv_buf, recv_buf_len);
    if (pMsg)
    {
        switch (pMsg->type)
        {
        case Arena::RESPONSE: handleResponse(pMsg); break;
        case Arena::HEARTBEAT: handleHeartbeat(pMsg); break;
        case Arena::NOTICE: handleNotice(pMsg); break;
        case Arena::DISBAND_LOBBY: handleDisbandLobby(pMsg); break;
        case Arena::PLAYER_JOINED_LOBBY: handlePlayerJoined(pMsg); break;
        case Arena::PLAYER_LEFT_LOBBY: handlePlayerLeft(pMsg); break;
        case Arena::CHECK_CHART_EXIST: handleCheckChartExist(pMsg); break;
        case Arena::HOST_REQUEST_CHART: handleHostRequestChart(pMsg); break;
        case Arena::HOST_READY_STAT: handleHostReadyStat(pMsg); break;
        case Arena::HOST_START_PLAYING: handleHostStartPlaying(pMsg); break;
        case Arena::HOST_PLAY_INIT: handleHostPlayInit(pMsg); break;
        case Arena::HOST_FINISHED_LOADING: handleHostFinishedLoading(pMsg); break;
        case Arena::HOST_PLAYDATA: handleHostPlayData(pMsg); break;
        case Arena::HOST_FINISHED_PLAYING: handleHostFinishedPlaying(pMsg); break;
        case Arena::HOST_FINISHED_RESULT: handleHostFinishedResult(pMsg); break;
        }
    }
    if (pMsg->type != Arena::RESPONSE)
        recvMessageIndex = std::max(recvMessageIndex, pMsg->messageIndex);
}

void ArenaClient::handleResponse(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageResponse>(msg);

    std::shared_lock l(tasksWaitingForResponseMutex);
    if (!tasksWaitingForResponse.contains(pMsg->messageIndex))
    {
        LOG_WARNING << "[Arena] Invalid req message index, or duplicate resp msg";
        return;
    }

    tasksWaitingForResponse[pMsg->messageIndex].received = true;

    if (pMsg->errorCode != 0)
    {
        LOG_WARNING << "[ArenaClient] Req type " << static_cast<int>(pMsg->reqType) << " error: ["
                    << static_cast<int>(pMsg->errorCode) << "] " << static_cast<ArenaErrorCode>(pMsg->errorCode);
        if (pMsg->reqType == Arena::ArenaMessageType::JOIN_LOBBY)
            joinLobbyErrorCode = pMsg->errorCode;
        return;
    }

    switch (pMsg->reqType)
    {
    case Arena::JOIN_LOBBY: handleJoinLobbyResp(pMsg); break;
    }
}

void ArenaClient::handleJoinLobbyResp(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageResponse>(msg);

    joinLobbyErrorCode = pMsg->errorCode;
    if (pMsg->errorCode == 0)
    {
        ArenaJoinLobbyResp p;
        std::stringstream ss;
        ss.write(reinterpret_cast<char*>(pMsg->payload.data()), pMsg->payload.size());
        try
        {
            cereal::PortableBinaryInputArchive ar(ss);
            ar(p);
        }
        catch (const cereal::Exception& e)
        {
            LOG_ERROR << "[ArenaClient] cereal exception: " << e.what();
            return;
        }

        playerID = p.playerID;
        for (const auto& [id, name] : p.playerName)
        {
            if (id == playerID)
                continue;
            gArenaData.data[id].name = name;
            gArenaData.playerIDs.push_back(id);
        }
        gArenaData.updateTexts();
    }
}

void ArenaClient::handleHeartbeat(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHeartbeat>(msg);

    heartbeatTime = lunaticvibes::Time::now();

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
}

void ArenaClient::handleNotice(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageNotice>(msg);

    switch (static_cast<ArenaMessageNotice::Format>(pMsg->format))
    {
        // unsafe lmao
    case ArenaMessageNotice::PLAIN: createNotification(i18n::s(pMsg->i18nIndex)); break;
    case ArenaMessageNotice::S1: createNotification((boost::format(i18n::s(pMsg->i18nIndex)) % pMsg->s1).str()); break;
    case ArenaMessageNotice::S2:
        createNotification((boost::format(i18n::s(pMsg->i18nIndex)) % pMsg->s1 % pMsg->s2).str());
        break;
    case ArenaMessageNotice::D1: createNotification((boost::format(i18n::s(pMsg->i18nIndex)) % pMsg->d1).str()); break;
    case ArenaMessageNotice::D2:
        createNotification((boost::format(i18n::s(pMsg->i18nIndex)) % pMsg->d1 % pMsg->d2).str());
        break;
    }

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
}

void ArenaClient::handleDisbandLobby(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageDisbandLobby>(msg);

    createNotification(i18n::s(i18nText::ARENA_LOBBY_DISBANDED));
    LOG_WARNING << "[Arena] Host disbanded";

    socket->close();
    socket.reset();

    gArenaData.expired = true;
}

void ArenaClient::handlePlayerJoined(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessagePlayerJoinedLobby>(msg);

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    if (gArenaData.data.contains(pMsg->playerID))
    {
        LOG_WARNING << "[Arena] player ID " << pMsg->playerID << " already exists, removing old";

        for (size_t i = 0; i < gArenaData.getPlayerCount(); ++i)
        {
            if (gArenaData.playerIDs[i] == pMsg->playerID)
            {
                gArenaData.playerIDs.erase(gArenaData.playerIDs.begin() + i);
                gArenaData.data.erase(pMsg->playerID);
                break;
            }
        }
    }

    gArenaData.data[pMsg->playerID].name = pMsg->playerName;
    gArenaData.playerIDs.push_back(pMsg->playerID);
    gArenaData.updateTexts();

    createNotification((boost::format(i18n::c(i18nText::ARENA_PLAYER_JOINED)) % pMsg->playerName).str());
}

void ArenaClient::handlePlayerLeft(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessagePlayerLeftLobby>(msg);

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    if (!gArenaData.data.contains(pMsg->playerID))
    {
        LOG_WARNING << "[Arena] player ID " << pMsg->playerID << " does not exist";
    }
    else
    {
        for (size_t i = 0; i < gArenaData.getPlayerCount(); ++i)
        {
            if (gArenaData.playerIDs[i] == pMsg->playerID)
            {
                createNotification(
                    (boost::format(i18n::c(i18nText::ARENA_PLAYER_LEFT)) % gArenaData.data[i].name).str());
                gArenaData.playerIDs.erase(gArenaData.playerIDs.begin() + i);
                gArenaData.data.erase(pMsg->playerID);
                break;
            }
        }
    }
}

void ArenaClient::handleCheckChartExist(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageCheckChartExist>(msg);

    ArenaMessageResponse resp(*pMsg);

    ArenaCheckChartExistResp subPayload;
    subPayload.exist = !g_pSongDB->findChartByHash(HashMD5{pMsg->chartHashMD5String}).empty();
    std::stringstream ss;
    try
    {
        cereal::PortableBinaryOutputArchive ar(ss);
        ar(subPayload);
    }
    catch (const cereal::Exception& e)
    {
        LOG_ERROR << "[ArenaClient] cereal exception: " << e.what();
    }
    size_t length = ss.tellp();
    if (length == 0)
    {
        LOG_WARNING << "[Arena] Pack CHECK_CHART_EXIST resp payload failed";
        return;
    }
    resp.payload.resize(length);
    ss.read(reinterpret_cast<char*>(resp.payload.data()), length);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
}

void ArenaClient::handleHostRequestChart(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostRequestChart>(msg);

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    if (!pMsg->chartHashMD5String.empty())
    {
        HashMD5 hash{pMsg->chartHashMD5String};
        // select chart
        gSelectContext.remoteRequestedPlayer = pMsg->requestPlayerName;
        gSelectContext.remoteRequestedChart = hash;
    }
    else
    {
        // remove ready stat
        if (gSelectContext.isInArenaRequest)
        {
            gSelectContext.isArenaCancellingRequest = true;
        }
        for (auto& [id, d] : gArenaData.data)
        {
            d.ready = false;
        }
    }
}

void ArenaClient::handleHostReadyStat(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostReadyStat>(msg);

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    // select chart
    for (auto& [id, ready] : pMsg->ready)
    {
        if (id == playerID)
            gArenaData.ready = !!ready;
        else if (auto it = gArenaData.data.find(id); it != gArenaData.data.end())
            it->second.ready = !!ready;
    }
}

void ArenaClient::handleHostStartPlaying(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostStartPlaying>(msg);

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    gArenaData.randomSeed = pMsg->randomSeed;
    gArenaData.initPlaying(RulesetType(pMsg->rulesetType));
    gSelectContext.isArenaReady = true;
}

void ArenaClient::handleHostPlayInit(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostPlayInit>(msg);

    ArenaMessageResponse resp(*pMsg);

    if (auto it = gArenaData.data.find(pMsg->playerID); it != gArenaData.data.end())
        it->second.ruleset->unpackInit(pMsg->rulesetPayload);
    else
        resp.errorCode = 2;

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
}

void ArenaClient::handleHostFinishedLoading(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostFinishedLoading>(msg);

    gArenaData.playStartTimeMs = pMsg->playStartTimeMs;
    gArenaData.startPlaying();

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
}

void ArenaClient::handleHostPlayData(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostPlayData>(msg);

    if (pMsg->messageIndex > recvMessageIndex)
    {
        for (auto& [id, payload] : pMsg->payload)
        {
            if (!gArenaData.data.contains(id))
                continue;
            gArenaData.data[id].ruleset->unpackFrame(payload);
        }
    }
}

void ArenaClient::handleHostFinishedPlaying(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostFinishedPlaying>(msg);

    gArenaData.playingFinished = true;

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
}

void ArenaClient::handleHostFinishedResult(const std::shared_ptr<ArenaMessage>& msg)
{
    auto pMsg = std::static_pointer_cast<ArenaMessageHostFinishedResult>(msg);

    ArenaMessageResponse resp(*pMsg);

    auto payload = resp.pack();
    socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));

    gSelectContext.isArenaReady = false;
    gArenaData.stopPlaying();

    requestChartHash.reset();
    _isLoadingFinished = false;
    _isCreatedRuleset = false;
    _isPlayingFinished = false;
    _isResultFinished = false;
}

void ArenaClient::update()
{
    auto now = lunaticvibes::Time::now();

    bool alive = true;

    // wait response timeout
    {
        std::set<int> taskHasResponse;
        {
            std::shared_lock l(tasksWaitingForResponseMutex);
            for (auto& [msgID, task] : tasksWaitingForResponse)
            {
                if (task.received)
                {
                    taskHasResponse.insert(msgID);
                }
                else if ((now - task.t).norm() > 5000)
                {
                    if (task.retryTimes > 3)
                    {
                        // server has dead. Removing
                        alive = false;
                        break;
                    }
                    else
                    {
                        task.retryTimes++;
                        task.t = now;
                        socket->async_send_to(boost::asio::buffer(*task.sentMessage), server,
                                              std::bind_front(emptyHandleSend, task.sentMessage));
                    }
                }
            }
        }
        {
            std::unique_lock l(tasksWaitingForResponseMutex);
            for (auto& msgID : taskHasResponse)
            {
                tasksWaitingForResponse.erase(msgID);
            }
        }
    }

    // heartbeat
    if ((now - heartbeatTime).norm() > 30000)
    {
        alive = false;
    }

    if (alive)
    {
        if (gArenaData.playing)
        {
            gArenaData.updateGlobals();

            // send gamedata
            auto n = std::make_shared<ArenaMessageClientPlayData>();
            n->messageIndex = ++sendMessageIndex;
            n->payload = vRulesetNetwork::packFrame(gPlayContext.ruleset[PLAYER_SLOT_PLAYER]);

            auto payload = n->pack();
            socket->async_send_to(boost::asio::buffer(*payload), server, std::bind_front(emptyHandleSend, payload));
            // no resp
        }
    }
    else
    {
        leaveLobby();
        createNotification(i18n::s(i18nText::ARENA_LEAVE_TIMEOUT));
    }
}
