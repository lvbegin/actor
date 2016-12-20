#ifndef RESTART_STRATEGY_H__
#define RESTART_STRATEGY_H__

#include <functional>

enum class RestartType { RESTART_ONE, RESTART_ALL, };

using  RestartStrategy = std::function<RestartType(void)>;

/* to define in cpp */
static RestartStrategy defaultRestartStrategy = [](void) { return RestartType::RESTART_ONE; };

#endif
