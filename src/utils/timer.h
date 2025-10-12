#pragma once
#include <SDL2/SDL.h>

typedef struct KTimer KTimer;

/// @brief Create a new timer that counts time from the point of creation
/// @return Timer struct to use with other ktimer_* functions
KTimer* ktimer_create();

/// @brief Get time in milliseconds
/// @param timer Timer to get the time from
/// @return Time in milliseconds
Sint64 ktimer_getTimeMillis(KTimer* timer);

/// @brief Get time in seconds as int
/// @param timer Timer to get the time from
/// @return Time in seconds
Sint64 ktimer_getTimeSeconds(KTimer* timer);

/// @brief Get time in seconds as float
/// @param timer Timer to get the time from
/// @return Time in seconds with fraction
float ktimer_getTimeFloat(KTimer* timer);

/// @brief Set the timer time to the provided value
/// @param timer Timer where the new time will be set
/// @param timeMillis Time to set in milliseconds
void ktimer_setTimeMillis(KTimer* timer, Sint64 timeMillis);

/// @brief Set the timer time to the provided value
/// @param timer Timer where the new time will be set
/// @param timeSeconds Time to set in seconds
void ktimer_setTimeSeconds(KTimer *timer, Sint64 timeSeconds);

/// @brief Set the timer time to the provided value.
/// @param timer Timer where the new time will be set
/// @param timeMillis Time to set in seconds
void ktimer_setTimeFloat(KTimer* timer, float time);

/// @brief Removes the timer and deallocates its memory. It's recommended to set the original pointer to NULL after this function.
/// @param timer Timer to remove
void ktimer_destroy(KTimer* timer);
