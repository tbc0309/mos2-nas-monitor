#pragma once
// Single translation-unit facade: state -> query plans -> snapshots -> scheduler.
#include "poll_schedule.h"
#include "startup_plan.h"
#include "telemetry/collector_state.h"
#include "telemetry/metric_queries.h"
#include "telemetry/snapshots.h"
#include "telemetry/scheduler.h"
