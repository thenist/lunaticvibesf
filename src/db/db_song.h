#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include <common/entry/entry_song.h>
#include <common/types.h>
#include <db/db_conn.h>

// FIXME: use "__root_folder" or something, currently something somewhere assumes empty string here.
inline const HashMD5 ROOT_FOLDER_HASH = md5({});

class SongDB : public SQLite
{
public:
    enum FolderType
    {
        FOLDER,
        SONG_BMS,

        CUSTOM_FOLDER,
        COURSE,
        CLASS,
    };

public:
    SongDB() = delete;
    SongDB(const char* path);
    SongDB(const Path& path) : SongDB(lunaticvibes::cs(path.u8string())) {}
    ~SongDB() override;
    SongDB(SongDB&) = delete;
    SongDB& operator=(SongDB&) = delete;

protected:
    bool asyncAddChartTask(const HashMD5& folder, const Path& path);
    bool removeChart(const Path& path, const HashMD5& parent);
    bool removeChart(const HashMD5& md5, const HashMD5& parent);

public:
    // Search from genre, version, artist, artist2, title, title2.
    [[nodiscard]] std::vector<std::shared_ptr<ChartFormatBase>> findChartByName(const HashMD5& folder,
                                                                                const std::string&,
                                                                                unsigned limit = 1000) const;
    // May contain duplicates.
    [[nodiscard]] std::vector<std::shared_ptr<ChartFormatBase>> findChartByHash(const HashMD5&,
                                                                                bool checksum = true) const;
    [[nodiscard]] std::vector<std::shared_ptr<ChartFormatBase>> findChartFromTime(const HashMD5& folder,
                                                                                  unsigned long long addTime) const;

protected:
    std::vector<std::vector<std::any>> songQueryPool;
    std::unordered_map<HashMD5, std::vector<size_t>> songQueryHashMap;
    std::unordered_map<HashMD5, std::vector<size_t>> songQueryParentMap;
    std::vector<std::vector<std::any>> folderQueryPool;
    std::unordered_map<HashMD5, std::vector<size_t>> folderQueryHashMap;
    std::unordered_map<HashMD5, std::vector<size_t>> folderQueryParentMap;

public:
    void prepareCache();
    void freeCache();

public:
    int initializeFolders(const std::vector<Path>& paths);
    int addSubFolder(Path path, const HashMD5& parent = ROOT_FOLDER_HASH);
    void waitLoadingFinish() const;
    [[nodiscard]] bool didLoadingFinish() const { return _asyncLoadJobs == _asyncLoadJobsDone; };
    int removeFolder(const HashMD5& hash, bool removeSong = false);

protected:
    int addNewFolder(const HashMD5& hash, const Path& path, const HashMD5& parent);
    int refreshExistingFolder(const HashMD5& hash, const Path& path, FolderType type);

public:
    HashMD5 getFolderParent(const HashMD5& folder) const;
    HashMD5 getFolderParent(const Path& path) const;
    std::pair<bool, Path> getFolderPath(const HashMD5& folder) const;
    HashMD5 getFolderHash(Path path) const;

    std::shared_ptr<EntryFolderRegular> browse(const HashMD5& root, bool recursive = true);
    std::shared_ptr<EntryFolderSong> browseSong(const HashMD5& root);
    std::shared_ptr<EntryFolderRegular> search(const HashMD5& root, const std::string& key);

    std::atomic<bool> _asyncLoadStopRequested = false;
    std::atomic<size_t> _asyncLoadJobs = 0;
    std::atomic<size_t> _asyncLoadJobsDone = 0;

public:
    std::atomic<int> addChartTaskCount = 0;
    std::atomic<int> addChartTaskFinishCount = 0;
    std::atomic<int> addChartSuccess = 0;
    std::atomic<int> addChartModified = 0;
    std::atomic<int> addChartDeleted = 0;

    std::shared_mutex addCurrentPathMutex;
    std::string addCurrentPath;
    void resetAddSummary();

    void stopLoading();
};
