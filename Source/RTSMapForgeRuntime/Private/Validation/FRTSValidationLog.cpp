#include "Validation/FRTSValidationLog.h"

FString FRTSFailureLogEntry::ToLogString() const
{
    return FString::Printf(TEXT("[Retry %d] [%s] %s: %s (Score: %.1f)"),
        RetryAttempt,
        *FRTSValidationLog::FailureClassToString(FailureClass),
        *PassName,
        *Reason,
        ScoreAtFailure);
}

ERTSFailureClass FRTSFailureLogEntry::Classify(const FRTSValidationIssue& Issue)
{
    if (Issue.Severity == ERTSValidationSeverity::Pass)
    {
        return ERTSFailureClass::None;
    }
    
    const FString& Pass = Issue.PassName;
    const FString& Reason = Issue.Reason;
    
    if (Pass == TEXT("Traversal"))
    {
        if (Reason.Contains(TEXT("UNREACHABLE")))
        {
            if (Reason.Contains(TEXT("Base")) && Reason.Contains(TEXT("Base")))
            {
                return ERTSFailureClass::BaseUnreachable;
            }
            else if (Reason.Contains(TEXT("Expansion")))
            {
                return ERTSFailureClass::ExpansionIsolated;
            }
            return ERTSFailureClass::TraversalBlocked;
        }
        if (Reason.Contains(TEXT("rush distance")) || Reason.Contains(TEXT("short")))
        {
            return ERTSFailureClass::RushTooShort;
        }
        return ERTSFailureClass::TraversalBlocked;
    }
    
    if (Pass == TEXT("Spawn"))
    {
        return ERTSFailureClass::SpawnAreaTooSmall;
    }
    
    if (Pass == TEXT("Choke"))
    {
        if (Reason.Contains(TEXT("No choke")) || Reason.Contains(TEXT("no choke")))
        {
            return ERTSFailureClass::NoChokes;
        }
        if (Reason.Contains(TEXT("Excessive")) || Reason.Contains(TEXT("many")))
        {
            return ERTSFailureClass::TooManyChokes;
        }
        return ERTSFailureClass::Unknown;
    }
    
    if (Pass == TEXT("Economy") || Pass == TEXT("ResourceAccessibility") || Pass == TEXT("ResourceSafety"))
    {
        return ERTSFailureClass::ResourceImbalance;
    }
    
    if (Pass == TEXT("Navmesh"))
    {
        return ERTSFailureClass::TraversalBlocked;
    }
    
    if (Pass == TEXT("Fairness"))
    {
        return ERTSFailureClass::OverallScoreLow;
    }
    
    return ERTSFailureClass::Unknown;
}

void FRTSValidationLog::LogFailure(const FRTSValidationIssue& Issue, int32 RetryAttempt, float ScoreAtFailure)
{
    FRTSFailureLogEntry Entry;
    Entry.FailureClass = FRTSFailureLogEntry::Classify(Issue);
    Entry.PassName = Issue.PassName;
    Entry.Reason = Issue.Reason;
    Entry.Severity = Issue.Severity;
    Entry.RetryAttempt = RetryAttempt;
    Entry.ScoreAtFailure = ScoreAtFailure;
    Entries.Add(Entry);
}

void FRTSValidationLog::LogSuccess(int32 RetryAttempt, float FinalScore)
{
    FRTSFailureLogEntry Entry;
    Entry.FailureClass = ERTSFailureClass::None;
    Entry.PassName = TEXT("Validation");
    Entry.Reason = FString::Printf(TEXT("PASSED after %d retries. Final score: %.1f"), RetryAttempt + 1, FinalScore);
    Entry.Severity = ERTSValidationSeverity::Pass;
    Entry.RetryAttempt = RetryAttempt;
    Entry.ScoreAtFailure = FinalScore;
    Entries.Add(Entry);
}

ERTSFailureClass FRTSValidationLog::GetDominantFailureClass() const
{
    TMap<ERTSFailureClass, int32> Counts;
    
    for (const auto& Entry : Entries)
    {
        if (Entry.FailureClass != ERTSFailureClass::None)
        {
            Counts.FindOrAdd(Entry.FailureClass)++;
        }
    }
    
    ERTSFailureClass Best = ERTSFailureClass::Unknown;
    int32 BestCount = 0;
    
    for (const auto& Pair : Counts)
    {
        if (Pair.Value > BestCount)
        {
            BestCount = Pair.Value;
            Best = Pair.Key;
        }
    }
    
    return Best;
}

FString FRTSValidationLog::GetRetryRecommendation() const
{
    ERTSFailureClass Dominant = GetDominantFailureClass();
    
    switch (Dominant)
    {
        case ERTSFailureClass::TraversalBlocked:
            return TEXT("RECOMMENDATION: Reduce WaterLevel (-0.05) or decrease river count. Map may have too many barriers.");
        
        case ERTSFailureClass::BaseUnreachable:
            return TEXT("RECOMMENDATION: Increase MinRushDistance or reduce SymmetryStrength. Bases may be placed on opposite sides of impassable terrain.");
        
        case ERTSFailureClass::ExpansionIsolated:
            return TEXT("RECOMMENDATION: Increase NumExpansions or reduce river width. Expansions may be cut off by water.");
        
        case ERTSFailureClass::NoChokes:
            return TEXT("RECOMMENDATION: Increase TerrainScale (rougher terrain) or decrease WaterLevel. Map may be too open.");
        
        case ERTSFailureClass::TooManyChokes:
            return TEXT("RECOMMENDATION: Decrease TerrainScale or increase WaterLevel. Map may be too fragmented.");
        
        case ERTSFailureClass::SpawnAreaTooSmall:
            return TEXT("RECOMMENDATION: Decrease MountainLevel or reduce Slope threshold. Terrain may be too rugged for bases.");
        
        case ERTSFailureClass::ResourceImbalance:
            return TEXT("RECOMMENDATION: Enable stronger symmetry or reduce biome count. Resource distribution may be asymmetric.");
        
        case ERTSFailureClass::RushTooShort:
            return TEXT("RECOMMENDATION: Increase map size or set MinRushDistance higher. Bases may be too close.");
        
        case ERTSFailureClass::RushTooLong:
            return TEXT("RECOMMENDATION: Decrease map size or open more passages. Bases may be too far apart.");
        
        case ERTSFailureClass::OverallScoreLow:
            return TEXT("RECOMMENDATION: Try different seed or adjust terrain parameters (FBMOctaves, Persistence). Map lacks strategic depth.");
        
        default:
            return TEXT("RECOMMENDATION: Try a different seed or adjust core parameters (TerrainScale, WaterLevel).");
    }
}

FString FRTSValidationLog::GenerateFullLog() const
{
    FString Log = TEXT("=== RTS MapForge Validation Log ===\n");
    
    for (const auto& Entry : Entries)
    {
        Log += Entry.ToLogString() + TEXT("\n");
    }
    
    if (HasCriticalFailures())
    {
        Log += TEXT("\n") + GetRetryRecommendation() + TEXT("\n");
    }
    
    Log += TEXT("===================================\n");
    return Log;
}

bool FRTSValidationLog::HasCriticalFailures() const
{
    for (const auto& Entry : Entries)
    {
        if (Entry.Severity == ERTSValidationSeverity::Critical)
        {
            return true;
        }
    }
    return false;
}

FString FRTSValidationLog::FailureClassToString(ERTSFailureClass Class)
{
    switch (Class)
    {
        case ERTSFailureClass::None:              return TEXT("PASS");
        case ERTSFailureClass::TraversalBlocked:   return TEXT("TRAVERSAL");
        case ERTSFailureClass::BaseUnreachable:    return TEXT("BASE_UNREACHABLE");
        case ERTSFailureClass::ExpansionIsolated:return TEXT("EXP_ISOLATED");
        case ERTSFailureClass::NoChokes:          return TEXT("NO_CHOKES");
        case ERTSFailureClass::TooManyChokes:     return TEXT("TOO_MANY_CHOKES");
        case ERTSFailureClass::SpawnAreaTooSmall: return TEXT("SPAWN_SMALL");
        case ERTSFailureClass::ResourceImbalance: return TEXT("RES_IMBALANCE");
        case ERTSFailureClass::RushTooShort:      return TEXT("RUSH_SHORT");
        case ERTSFailureClass::RushTooLong:       return TEXT("RUSH_LONG");
        case ERTSFailureClass::OverallScoreLow:  return TEXT("SCORE_LOW");
        case ERTSFailureClass::Unknown:          return TEXT("UNKNOWN");
        default:                                 return TEXT("?");
    }
}
