#include "cmd-tracing.h"
#include "debugger.h"
#include "historycontext.h"
#include "threading.h"
#include "module.h"
#include "console.h"
#include "cmd-debug-control.h"
#include "value.h"

extern std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;

static bool cbDebugConditionalTrace(void* callBack, bool stepOver, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(dbgtraceactive())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Trace already active"));
        return false;
    }
    duint maxCount;
    if(!BridgeSettingGetUint("Engine", "MaxTraceCount", &maxCount) || !maxCount)
        maxCount = 50000;
    if(argc > 2 && !valfromstring(argv[2], &maxCount, false))
        return false;
    if(!dbgsettracecondition(*argv[1] ? argv[1] : "0", maxCount))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), argv[1]);
        return false;
    }
    HistoryClear();
    if(stepOver)
        StepOver(callBack);
    else
        StepInto(callBack);
    return cbDebugRunInternal(argc, argv);
}

bool cbDebugTraceIntoConditional(int argc, char* argv[])
{
    return cbDebugConditionalTrace((void*)cbTraceIntoConditionalStep, false, argc, argv);
}

bool cbDebugTraceOverConditional(int argc, char* argv[])
{
    return cbDebugConditionalTrace((void*)cbTraceOverConditionalStep, true, argc, argv);
}

bool cbDebugTraceIntoBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tibt", "0" };
        return cbDebugConditionalTrace((void*)cbTraceIntoBeyondTraceRecordStep, false, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTraceIntoBeyondTraceRecordStep, false, argc, argv);
}

bool cbDebugTraceOverBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tobt", "0" };
        return cbDebugConditionalTrace((void*)cbTraceOverBeyondTraceRecordStep, true, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTraceOverBeyondTraceRecordStep, true, argc, argv);
}

bool cbDebugTraceIntoIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tiit", "0" };
        return cbDebugConditionalTrace((void*)cbTraceIntoIntoTraceRecordStep, false, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTraceIntoIntoTraceRecordStep, false, argc, argv);
}

bool cbDebugTraceOverIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "toit", "0" };
        return cbDebugConditionalTrace((void*)cbTraceOverIntoTraceRecordStep, true, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTraceOverIntoTraceRecordStep, true, argc, argv);
}

bool cbDebugRunToParty(int argc, char* argv[])
{
    HistoryClear();
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    std::vector<MODINFO> AllModules;
    ModGetList(AllModules);
    if(!RunToUserCodeBreakpoints.empty())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Run to party is busy.\n"));
        return false;
    }
    int party = atoi(argv[1]); // party is a signed integer
    for(auto i : AllModules)
    {
        if(i.party == party)
        {
            for(auto j : i.sections)
            {
                BREAKPOINT bp;
                if(!BpGet(j.addr, BPMEMORY, nullptr, &bp))
                {
                    RunToUserCodeBreakpoints.push_back(std::make_pair(j.addr, j.size));
                    SetMemoryBPXEx(j.addr, j.size, UE_MEMORY_EXECUTE, false, (void*)cbRunToUserCodeBreakpoint);
                }
            }
        }
    }
    return cbDebugRunInternal(argc, argv);
}

bool cbDebugRunToUserCode(int argc, char* argv[])
{
    char* newargv[] = { "RunToParty", "0" };
    return cbDebugRunToParty(argc, newargv);
}

bool cbDebugTraceSetLog(int argc, char* argv[])
{
    auto text = argc > 1 ? argv[1] : "";
    auto condition = argc > 2 ? argv[2] : "";
    if(!dbgsettracelog(condition, text))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), condition);
        return false;
    }
    return true;
}

bool cbDebugTraceSetCommand(int argc, char* argv[])
{
    auto text = argc > 1 ? argv[1] : "";
    auto condition = argc > 2 ? argv[2] : "";
    if(!dbgsettracecmd(condition, text))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), condition);
        return false;
    }
    return true;
}
