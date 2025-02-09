#pragma once

#include <common/beat.h>
#include <common/types.h>

#include <list>
#include <shared_mutex>
#include <utility>

struct OverlayContextParams
{
    std::shared_mutex _mutex;
    std::list<std::pair<lunaticvibes::Time, StringContent>> notifications;
};

void createNotification(StringContent text);
