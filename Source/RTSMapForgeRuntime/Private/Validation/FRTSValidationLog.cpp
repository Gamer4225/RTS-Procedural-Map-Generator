#include "Validation/FRTSValidationLog.h"

FString FRTSFailureLogEntry::ToLogString() const
{
    return FString::Printf(TEXT("[Retry %d] [%s] %s: %s (Score: %.1f)"), RetryAttempt, *FRTSValidationLog::FailureClassToString(FailureClass), *PassName, *Reason, ScoreAtFailure);
}

ERTSFailureClass FRTSFailureLogEntry::Classify(const FRTSValidationIssue& Issue)
{
    if (Issue.Severity==ERTSValidationSeverity::Pass) return ERTSFailureClass::None;
    if (Issue.PassName==TEXT("Traversal"))
    {
        if (Issue.Reason.Contains(TEXT("UNREACHABLE"))) { if (Issue.Reason.Contains(TEXT("Base"))) return ERTSFailureClass::BaseUnreachable; if (Issue.Reason.Contains(TEXT("Expansion"))) return ERTSFailureClass::ExpansionIsolated; return ERTSFailureClass::TraversalBlocked; }
        if (Issue.Reason.Contains(TEXT("short"))) return ERTSFailureClass::RushTooShort;
        return ERTSFailureClass::TraversalBlocked;
    }
    if (Issue.PassName==TEXT("Spawn")) return ERTSFailureClass::SpawnAreaTooSmall;
    if (Issue.PassName==TEXT("Choke")) { if (Issue.Reason.Contains(TEXT("No choke"))) return ERTSFailureClass::NoChokes; if (Issue.Reason.Contains(TEXT("many"))) return ERTSFailureClass::TooManyChokes; }
    if (Issue.PassName==TEXT("Economy")||Issue.PassName==TEXT("ResourceAccessibility")||Issue.PassName==TEXT("ResourceSafety")) return ERTSFailureClass::ResourceImbalance;
    if (Issue.PassName==TEXT("Fairness")) return ERTSFailureClass::OverallScoreLow;
    return ERTSFailureClass::Unknown;
}

void FRTSValidationLog::LogFailure(const FRTSValidationIssue& Issue, int32 Retry, float Score) { FRTSFailureLogEntry E; E.FailureClass=FRTSFailureLogEntry::Classify(Issue); E.PassName=Issue.PassName; E.Reason=Issue.Reason; E.Severity=Issue.Severity; E.RetryAttempt=Retry; E.ScoreAtFailure=Score; Entries.Add(E); }
void FRTSValidationLog::LogSuccess(int32 Retry, float Score) { FRTSFailureLogEntry E; E.PassName=TEXT("Validation"); E.Reason=FString::Printf(TEXT("PASSED after %d retries. Score: %.1f"),Retry+1,Score); E.RetryAttempt=Retry; E.ScoreAtFailure=Score; Entries.Add(E); }
ERTSFailureClass FRTSValidationLog::GetDominantFailureClass() const { TMap<ERTSFailureClass,int32> Cnt; for (const auto& E:Entries) if (E.FailureClass!=ERTSFailureClass::None) Cnt.FindOrAdd(E.FailureClass)++; ERTSFailureClass Best=ERTSFailureClass::Unknown; int32 BC=0; for (const auto& P:Cnt) if (P.Value>BC){BC=P.Value;Best=P.Key;} return Best; }
FString FRTSValidationLog::GetRetryRecommendation() const { return TEXT("RECOMMENDATION: Try a different seed or adjust TerrainScale / WaterLevel."); }
FString FRTSValidationLog::GenerateFullLog() const { FString L=TEXT("=== RTSMapForge Validation Log ===\n"); for (const auto& E:Entries) L+=E.ToLogString()+TEXT("\n"); L+=TEXT("===================================\n"); return L; }
bool FRTSValidationLog::HasCriticalFailures() const { for (const auto& E:Entries) if (E.Severity==ERTSValidationSeverity::Critical) return true; return false; }
FString FRTSValidationLog::FailureClassToString(ERTSFailureClass C)
{
    switch(C) { case ERTSFailureClass::None: return TEXT("PASS"); case ERTSFailureClass::TraversalBlocked: return TEXT("TRAVERSAL"); case ERTSFailureClass::BaseUnreachable: return TEXT("BASE_UNREACHABLE"); case ERTSFailureClass::NoChokes: return TEXT("NO_CHOKES"); case ERTSFailureClass::ResourceImbalance: return TEXT("RES_IMBALANCE"); case ERTSFailureClass::OverallScoreLow: return TEXT("SCORE_LOW"); default: return TEXT("UNKNOWN"); }
}
