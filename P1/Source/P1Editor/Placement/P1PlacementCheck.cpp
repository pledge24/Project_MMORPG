#include "Placement/P1PlacementCheck.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    using FPlacedBox = FP1PlacementCheck::FPlacedBox;
    using FPlacedArea = FP1PlacementCheck::FPlacedArea;
    using FPlacedPoint = FP1PlacementCheck::FPlacedPoint;
    using FLevel = FP1PlacementCheck::FLevel;
    using FDesignData = FP1PlacementCheck::FDesignData;

    /** 경계 벽과 지역 볼륨을 같다고 보는 차이(cm). 에디터가 위치를 double에서 float로 옮기며 생기는 오차를 넘는다. */
    constexpr double WALL_TOLERANCE = 1.0;

    //~ 포함 판정. 서버의 PlaneBounds와 같이 경계 위의 점은 안쪽이다.

    bool ContainsPoint(const FPlacedBox& Box, const FVector& Point)
    {
        return FMath::Abs(Point.X - Box.Center.X) <= Box.HalfExtent.X && FMath::Abs(Point.Y - Box.Center.Y) <= Box.HalfExtent.Y;
    }

    bool ContainsBox(const FPlacedBox& Outer, const FPlacedBox& Inner)
    {
        return Inner.Center.X - Inner.HalfExtent.X >= Outer.Center.X - Outer.HalfExtent.X
            && Inner.Center.X + Inner.HalfExtent.X <= Outer.Center.X + Outer.HalfExtent.X
            && Inner.Center.Y - Inner.HalfExtent.Y >= Outer.Center.Y - Outer.HalfExtent.Y
            && Inner.Center.Y + Inner.HalfExtent.Y <= Outer.Center.Y + Outer.HalfExtent.Y;
    }

    /** 안쪽이 겹치면 참이다. 경계만 함께 쓰는 두 상자는 겹치지 않는다. */
    bool Overlaps(const FPlacedBox& A, const FPlacedBox& B)
    {
        return FMath::Abs(A.Center.X - B.Center.X) < A.HalfExtent.X + B.HalfExtent.X
            && FMath::Abs(A.Center.Y - B.Center.Y) < A.HalfExtent.Y + B.HalfExtent.Y;
    }

    //~ 산출물

    /**
     * 소수점 둘째 자리에서 반올림하고 끝의 0을 지운다. 정수도 소수점 한 자리를 남긴다(1500.0).
     * 에디터에서 옮기지 않은 마커가 float 오차로 diff를 만들지 않게 한다.
     */
    FString FormatNumber(double Value)
    {
        double Rounded = FMath::RoundToDouble(Value * 100.0) / 100.0;
        if (Rounded == 0.0)
            Rounded = 0.0; // -0.0을 0.0으로 쓴다.

        FString Text = FString::Printf(TEXT("%.2f"), Rounded);
        while (Text.EndsWith(TEXT("0")) && Text.EndsWith(TEXT(".0")) == false)
            Text.LeftChopInline(1);
        return Text;
    }

    /** 이름과 값의 쌍을 JSON 객체 한 줄로 쓴다. 값은 이미 JSON 표기다. */
    FString JoinFields(const TArray<TPair<FString, FString>>& Fields)
    {
        TArray<FString> Parts;
        for (const TPair<FString, FString>& Field : Fields)
            Parts.Add(FString::Printf(TEXT("\"%s\":%s"), *Field.Key, *Field.Value));
        return FString::Join(Parts, TEXT(","));
    }

    /** 한 행이 한 줄인 배열. 행이 없으면 []. */
    FString JoinRows(const TArray<FString>& Rows)
    {
        if (Rows.IsEmpty())
            return TEXT("[]");
        return TEXT("[\n{") + FString::Join(Rows, TEXT("},\n{")) + TEXT("}\n]");
    }

    FString WriteFile(const FLevel& Level, const FP1PlacementCheck::FPlacedMap& Map, const TArray<const FPlacedArea*>& Areas,
        const TArray<TPair<int32, const FPlacedPoint*>>& RespawnPoints, const TArray<TPair<const FPlacedPoint*, const FPlacedPoint*>>& Portals)
    {
        const FPlacedBox& Bounds = Map.Bounds;
        const FString Header = JoinFields({
            { TEXT("MapId"), FString::FromInt(Level.MapId) },
            { TEXT("CenterX"), FormatNumber(Bounds.Center.X) }, { TEXT("CenterY"), FormatNumber(Bounds.Center.Y) },
            { TEXT("HalfExtentX"), FormatNumber(Bounds.HalfExtent.X) }, { TEXT("HalfExtentY"), FormatNumber(Bounds.HalfExtent.Y) },
            { TEXT("CellSize"), FormatNumber(Map.CellSize) },
        });

        TArray<FString> AreaRows;
        for (const FPlacedArea* Area : Areas)
        {
            AreaRows.Add(JoinFields({
                { TEXT("Id"), FString::FromInt(Area->AreaId) },
                { TEXT("CenterX"), FormatNumber(Area->Box.Center.X) }, { TEXT("CenterY"), FormatNumber(Area->Box.Center.Y) },
                { TEXT("CenterZ"), FormatNumber(Area->Box.Center.Z) },
                { TEXT("HalfExtentX"), FormatNumber(Area->Box.HalfExtent.X) }, { TEXT("HalfExtentY"), FormatNumber(Area->Box.HalfExtent.Y) },
            }));
        }

        TArray<FString> RespawnRows;
        for (const TPair<int32, const FPlacedPoint*>& Respawn : RespawnPoints)
        {
            const FPlacedPoint& Point = *Respawn.Value;
            RespawnRows.Add(JoinFields({
                { TEXT("AreaId"), FString::FromInt(Respawn.Key) },
                { TEXT("X"), FormatNumber(Point.Location.X) }, { TEXT("Y"), FormatNumber(Point.Location.Y) },
                { TEXT("Z"), FormatNumber(Point.Location.Z) }, { TEXT("Yaw"), FormatNumber(Point.Yaw) },
            }));
        }

        TArray<FString> PortalRows;
        for (const TPair<const FPlacedPoint*, const FPlacedPoint*>& Portal : Portals)
        {
            const FPlacedPoint& Src = *Portal.Key;
            const FPlacedPoint& Dest = *Portal.Value;
            PortalRows.Add(JoinFields({
                { TEXT("Id"), FString::FromInt(Src.Id) },
                { TEXT("SrcX"), FormatNumber(Src.Location.X) }, { TEXT("SrcY"), FormatNumber(Src.Location.Y) },
                { TEXT("SrcZ"), FormatNumber(Src.Location.Z) }, { TEXT("Radius"), FormatNumber(Src.Radius) },
                { TEXT("DestX"), FormatNumber(Dest.Location.X) }, { TEXT("DestY"), FormatNumber(Dest.Location.Y) },
                { TEXT("DestZ"), FormatNumber(Dest.Location.Z) }, { TEXT("DestYaw"), FormatNumber(Dest.Yaw) },
            }));
        }

        return FString::Printf(TEXT("{%s,\n\"Areas\":%s,\n\"RespawnPoints\":%s,\n\"Portals\":%s}\n"),
            *Header, *JoinRows(AreaRows), *JoinRows(RespawnRows), *JoinRows(PortalRows));
    }

    //~ 기획 데이터

    bool ReadRows(const FString& Json, const FString& FileName, TArray<TSharedPtr<FJsonValue>>& OutRows, TArray<FString>& OutErrors)
    {
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
        if (FJsonSerializer::Deserialize(Reader, OutRows) == false)
        {
            OutErrors.Add(FString::Printf(TEXT("%s: 행 배열로 읽지 못했다: %s"), *FileName, *Reader->GetErrorMessage()));
            return false;
        }
        return true;
    }

    /** 행 객체의 정수 필드. 없으면 OutErrors에 더하고 false. */
    bool ReadInt(const TSharedPtr<FJsonObject>& Row, const FString& Field, const FString& Where, int32& OutValue, TArray<FString>& OutErrors)
    {
        if (Row.IsValid() && Row->TryGetNumberField(Field, OutValue))
            return true;
        OutErrors.Add(FString::Printf(TEXT("%s: '%s'가 없다"), *Where, *Field));
        return false;
    }
}

bool FP1PlacementCheck::ParseDesignData(const FString& MapJson, const FString& AreaJson, FDesignData& OutDesign, TArray<FString>& OutErrors)
{
    const int32 ErrorCount = OutErrors.Num();

    TArray<TSharedPtr<FJsonValue>> MapRows;
    if (ReadRows(MapJson, TEXT("Map.json"), MapRows, OutErrors))
    {
        for (int32 Index = 0; Index < MapRows.Num(); ++Index)
        {
            int32 MapId = 0;
            if (ReadInt(MapRows[Index]->AsObject(), TEXT("Id"), FString::Printf(TEXT("Map.json %d번째 행"), Index + 1), MapId, OutErrors))
                OutDesign.MapIds.Add(MapId);
        }
    }

    TArray<TSharedPtr<FJsonValue>> AreaRows;
    if (ReadRows(AreaJson, TEXT("Area.json"), AreaRows, OutErrors))
    {
        for (int32 Index = 0; Index < AreaRows.Num(); ++Index)
        {
            const TSharedPtr<FJsonObject> Row = AreaRows[Index]->AsObject();
            const FString Where = FString::Printf(TEXT("Area.json %d번째 행"), Index + 1);
            int32 AreaId = 0;
            FDesignData::FArea Area;
            bool bHasRespawnPoint = false;
            const TArray<TSharedPtr<FJsonValue>>* Portals = nullptr;
            if (ReadInt(Row, TEXT("Id"), Where, AreaId, OutErrors) == false || ReadInt(Row, TEXT("MapId"), Where, Area.MapId, OutErrors) == false)
                continue;
            if (Row->TryGetBoolField(TEXT("HasRespawnPoint"), bHasRespawnPoint) == false || Row->TryGetArrayField(TEXT("Portals"), Portals) == false)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: 'HasRespawnPoint'나 'Portals'가 없다"), *Where));
                continue;
            }

            Area.bHasRespawnPoint = bHasRespawnPoint;
            OutDesign.Areas.Add(AreaId, Area);
            for (int32 PortalIndex = 0; PortalIndex < Portals->Num(); ++PortalIndex)
            {
                const FString PortalWhere = FString::Printf(TEXT("%s 'Portals' %d번째 원소"), *Where, PortalIndex + 1);
                int32 PortalId = 0;
                FDesignData::FPortal Portal;
                Portal.AreaId = AreaId;
                const TSharedPtr<FJsonObject> PortalRow = (*Portals)[PortalIndex]->AsObject();
                if (ReadInt(PortalRow, TEXT("Id"), PortalWhere, PortalId, OutErrors) && ReadInt(PortalRow, TEXT("DestAreaId"), PortalWhere, Portal.DestAreaId, OutErrors))
                    OutDesign.Portals.Add(PortalId, Portal);
            }
        }
    }

    return OutErrors.Num() == ErrorCount;
}

FP1PlacementCheck::FResult FP1PlacementCheck::Run(const TArray<FLevel>& Levels, const FDesignData& Design)
{
    FResult Result;
    TArray<FString>& Errors = Result.Errors;
    auto Error = [&Errors](const FString& Message) { Errors.Add(Message); };

    // 모든 레벨의 지역 볼륨과 도착 지점. 다른 맵으로 가는 포털의 도착 지점이 도착 지역 안인지 보려고 레벨을 넘어 모은다.
    TMap<int32, TPair<const FLevel*, const FPlacedArea*>> AreaById;
    TMap<int32, TPair<const FLevel*, const FPlacedPoint*>> ArrivalById;
    TSet<int32> ExtractedMapIds;

    // 맵마다 산출물에 들어갈 행. 레벨을 다 본 뒤 오류가 없을 때만 쓴다.
    struct FMapRows
    {
        const FLevel* Level = nullptr;
        const FPlacedMap* Map = nullptr;
        TArray<const FPlacedArea*> Areas;
        TArray<TPair<int32, const FPlacedPoint*>> RespawnPoints;
        TArray<const FPlacedPoint*> Portals;
    };
    TArray<FMapRows> MapRows;

    // 1) 레벨 하나 안에서 끝나는 검사.
    for (const FLevel& Level : Levels)
    {
        const FString& LevelName = Level.LevelName;
        FMapRows& Rows = MapRows.AddDefaulted_GetRef();
        Rows.Level = &Level;
        ExtractedMapIds.Add(Level.MapId);

        if (Design.MapIds.Contains(Level.MapId) == false)
            Error(FString::Printf(TEXT("%s: 맵 %d이 맵 표에 없다"), *LevelName, Level.MapId));

        if (Level.MapMarkers.Num() != 1)
        {
            Error(FString::Printf(TEXT("%s: 맵 마커가 %d개다. 레벨에 맵 마커를 하나 둔다"), *LevelName, Level.MapMarkers.Num()));
        }
        else
        {
            Rows.Map = &Level.MapMarkers[0];
            const FPlacedBox& Bounds = Rows.Map->Bounds;
            if (Bounds.bRotated)
                Error(FString::Printf(TEXT("%s %s: 맵 마커가 회전해 있다. 서버는 축에 맞춘 경계만 판정한다"), *LevelName, *Bounds.Name));
            if (Bounds.HalfExtent.X <= 0.0 || Bounds.HalfExtent.Y <= 0.0)
                Error(FString::Printf(TEXT("%s %s: 맵 경계의 반경이 0보다 커야 한다"), *LevelName, *Bounds.Name));
            if (Rows.Map->CellSize <= 0.f)
                Error(FString::Printf(TEXT("%s %s: 셀 크기가 0보다 커야 하는데 %s이다"), *LevelName, *Bounds.Name, *FormatNumber(Rows.Map->CellSize)));
        }

        auto CheckInsideMap = [&](const FString& Name, const FVector& Point)
        {
            if (Rows.Map != nullptr && ContainsPoint(Rows.Map->Bounds, Point) == false)
                Error(FString::Printf(TEXT("%s %s: 맵 경계 밖에 있다"), *LevelName, *Name));
        };

        // 지역 볼륨.
        for (const FPlacedArea& Area : Level.Areas)
        {
            const FString& Name = Area.Box.Name;
            if (Area.Box.bRotated)
                Error(FString::Printf(TEXT("%s %s: 지역 볼륨이 회전해 있다. 서버는 축에 맞춘 상자로만 판정한다"), *LevelName, *Name));
            if (Area.Box.HalfExtent.X <= 0.0 || Area.Box.HalfExtent.Y <= 0.0)
                Error(FString::Printf(TEXT("%s %s: 지역 볼륨의 반경이 0보다 커야 한다"), *LevelName, *Name));
            if (Rows.Map != nullptr && ContainsBox(Rows.Map->Bounds, Area.Box) == false)
                Error(FString::Printf(TEXT("%s %s: 지역 볼륨이 맵 경계 밖으로 나간다"), *LevelName, *Name));

            const FDesignData::FArea* DesignArea = Design.Areas.Find(Area.AreaId);
            if (DesignArea == nullptr)
            {
                Error(FString::Printf(TEXT("%s %s: 지역 %d이 지역 표에 없다"), *LevelName, *Name, Area.AreaId));
                continue;
            }
            if (DesignArea->MapId != Level.MapId)
                Error(FString::Printf(TEXT("%s %s: 지역 %d은 지역 표에서 맵 %d의 지역이다"), *LevelName, *Name, Area.AreaId, DesignArea->MapId));

            if (const TPair<const FLevel*, const FPlacedArea*>* Other = AreaById.Find(Area.AreaId))
            {
                Error(FString::Printf(TEXT("%s %s: 지역 %d의 볼륨이 %s에도 있다"), *LevelName, *Name, Area.AreaId, *Other->Value->Box.Name));
                continue;
            }
            AreaById.Add(Area.AreaId, { &Level, &Area });
            Rows.Areas.Add(&Area);
        }

        for (int32 I = 0; I < Level.Areas.Num(); ++I)
        {
            for (int32 J = I + 1; J < Level.Areas.Num(); ++J)
            {
                if (Overlaps(Level.Areas[I].Box, Level.Areas[J].Box))
                    Error(FString::Printf(TEXT("%s: %s와 %s가 겹친다. 지역은 경계만 함께 쓸 수 있다"), *LevelName, *Level.Areas[I].Box.Name, *Level.Areas[J].Box.Name));
            }
        }

        for (const TPair<int32, FDesignData::FArea>& DesignArea : Design.Areas)
        {
            if (DesignArea.Value.MapId == Level.MapId && AreaById.Contains(DesignArea.Key) == false)
                Error(FString::Printf(TEXT("%s: 지역 %d의 지역 볼륨이 없다"), *LevelName, DesignArea.Key));
        }

        // 그 레벨에서 점이 드는 지역. 지역끼리 겹치면 위에서 이미 오류다.
        auto FindAreaContaining = [&Rows](const FVector& Point) -> const FPlacedArea*
        {
            for (const FPlacedArea* Area : Rows.Areas)
            {
                if (ContainsPoint(Area->Box, Point))
                    return Area;
            }
            return nullptr;
        };

        // 리스폰 지점. 소속 지역은 포함 판정으로 정한다.
        TMap<int32, const FPlacedPoint*> RespawnByArea;
        for (const FPlacedPoint& Respawn : Level.RespawnPoints)
        {
            CheckInsideMap(Respawn.Name, Respawn.Location);
            const FPlacedArea* Area = FindAreaContaining(Respawn.Location);
            if (Area == nullptr)
            {
                Error(FString::Printf(TEXT("%s %s: 리스폰 지점이 어느 지역 볼륨에도 들지 않는다"), *LevelName, *Respawn.Name));
                continue;
            }

            const FDesignData::FArea& DesignArea = Design.Areas[Area->AreaId];
            if (DesignArea.bHasRespawnPoint == false)
                Error(FString::Printf(TEXT("%s %s: 리스폰 지점이 지역 표에서 리스폰 지점이 없는 지역 %d에 있다"), *LevelName, *Respawn.Name, Area->AreaId));
            else if (const FPlacedPoint* const* Other = RespawnByArea.Find(Area->AreaId))
                Error(FString::Printf(TEXT("%s %s: 지역 %d에 리스폰 지점이 %s와 둘이다"), *LevelName, *Respawn.Name, Area->AreaId, *(*Other)->Name));
            else
                RespawnByArea.Add(Area->AreaId, &Respawn);
        }

        for (const TPair<int32, FDesignData::FArea>& DesignArea : Design.Areas)
        {
            if (DesignArea.Value.MapId == Level.MapId && DesignArea.Value.bHasRespawnPoint && RespawnByArea.Contains(DesignArea.Key) == false)
                Error(FString::Printf(TEXT("%s: 지역 %d에 리스폰 지점이 없다. 지역 표에서 리스폰 지점이 있는 지역이다"), *LevelName, DesignArea.Key));
        }
        for (const TPair<int32, const FPlacedPoint*>& Respawn : RespawnByArea)
            Rows.RespawnPoints.Add({ Respawn.Key, Respawn.Value });

        // 포털의 출발 위치. 출발 지역은 포털 표가 정한다.
        TSet<int32> PortalIds;
        for (const FPlacedPoint& Portal : Level.Portals)
        {
            CheckInsideMap(Portal.Name, Portal.Location);
            const FDesignData::FPortal* DesignPortal = Design.Portals.Find(Portal.Id);
            if (DesignPortal == nullptr)
            {
                Error(FString::Printf(TEXT("%s %s: 포털 %d이 포털 표에 없다"), *LevelName, *Portal.Name, Portal.Id));
                continue;
            }
            if (PortalIds.Contains(Portal.Id))
            {
                Error(FString::Printf(TEXT("%s %s: 포털 %d이 레벨에 둘 이상 있다"), *LevelName, *Portal.Name, Portal.Id));
                continue;
            }
            PortalIds.Add(Portal.Id);

            if (Portal.Radius <= 0.0)
                Error(FString::Printf(TEXT("%s %s: 포털의 반경이 0보다 커야 한다"), *LevelName, *Portal.Name));

            const TPair<const FLevel*, const FPlacedArea*>* SourceArea = AreaById.Find(DesignPortal->AreaId);
            if (SourceArea == nullptr || SourceArea->Key != &Level)
                Error(FString::Printf(TEXT("%s %s: 포털 %d의 출발 지역 %d이 이 레벨에 없다"), *LevelName, *Portal.Name, Portal.Id, DesignPortal->AreaId));
            else if (ContainsPoint(SourceArea->Value->Box, Portal.Location) == false)
                Error(FString::Printf(TEXT("%s %s: 포털 %d이 출발 지역 %d 밖에 있다"), *LevelName, *Portal.Name, Portal.Id, DesignPortal->AreaId));

            Rows.Portals.Add(&Portal);
        }

        for (const TPair<int32, FDesignData::FPortal>& DesignPortal : Design.Portals)
        {
            const FDesignData::FArea* Owner = Design.Areas.Find(DesignPortal.Value.AreaId);
            if (Owner != nullptr && Owner->MapId == Level.MapId && PortalIds.Contains(DesignPortal.Key) == false)
                Error(FString::Printf(TEXT("%s: 포털 %d이 레벨에 없다"), *LevelName, DesignPortal.Key));
        }

        // 포털의 도착 지점. 도착 지역 검사는 모든 레벨을 본 뒤에 한다.
        for (const FPlacedPoint& Arrival : Level.Arrivals)
        {
            CheckInsideMap(Arrival.Name, Arrival.Location);
            if (Design.Portals.Contains(Arrival.Id) == false)
                Error(FString::Printf(TEXT("%s %s: 도착 지점의 포털 %d이 포털 표에 없다"), *LevelName, *Arrival.Name, Arrival.Id));
            else if (const TPair<const FLevel*, const FPlacedPoint*>* Other = ArrivalById.Find(Arrival.Id))
                Error(FString::Printf(TEXT("%s %s: 포털 %d의 도착 지점이 %s와 둘이다"), *LevelName, *Arrival.Name, Arrival.Id, *Other->Value->Name));
            else
                ArrivalById.Add(Arrival.Id, { &Level, &Arrival });
        }

        // 경계 벽은 지역 볼륨의 크기로 맞춘다. 볼륨을 옮기고 벽을 다시 맞추지 않으면 클라이언트가 서버와 다른 곳에서 막힌다.
        for (const FP1PlacementCheck::FPlacedWall& Wall : Level.Walls)
        {
            const TPair<const FLevel*, const FPlacedArea*>* Area = AreaById.Find(Wall.AreaId);
            if (Wall.AreaId == 0 || Area == nullptr || Area->Key != &Level)
            {
                Error(FString::Printf(TEXT("%s %s: 경계 벽이 이 레벨의 지역 볼륨을 가리키지 않는다"), *LevelName, *Wall.Name));
                continue;
            }

            const FPlacedBox& Box = Area->Value->Box;
            const bool bSameCenter = FVector2D(Wall.Center.X - Box.Center.X, Wall.Center.Y - Box.Center.Y).GetAbsMax() <= WALL_TOLERANCE;
            const bool bSameExtent = (Wall.HalfExtent - Box.HalfExtent).GetAbsMax() <= WALL_TOLERANCE;
            if (bSameCenter == false || bSameExtent == false)
                Error(FString::Printf(TEXT("%s %s: 경계 벽이 지역 볼륨 %s와 위치나 크기가 다르다. 벽을 선택해 다시 맞춘다"), *LevelName, *Wall.Name, *Box.Name));
        }
    }

    // 2) 레벨을 넘는 검사. 맵 표의 맵마다 레벨이 있어야 서버가 그 맵의 배치 데이터를 받는다.
    TArray<int32> DesignMapIds = Design.MapIds.Array();
    DesignMapIds.Sort();
    for (int32 MapId : DesignMapIds)
    {
        if (ExtractedMapIds.Contains(MapId) == false)
            Error(FString::Printf(TEXT("맵 %d의 레벨이 없다. 프로젝트 설정 「P1 Game Data」의 맵 표(MapLevels)에 레벨을 더한다"), MapId));
    }

    // 포털의 도착 지점은 도착 지역의 맵 레벨에서 그 지역 안에 있어야 한다.
    for (const TPair<int32, FDesignData::FPortal>& DesignPortal : Design.Portals)
    {
        const FDesignData::FArea* Owner = Design.Areas.Find(DesignPortal.Value.AreaId);
        if (Owner == nullptr || ExtractedMapIds.Contains(Owner->MapId) == false)
            continue;

        const TPair<const FLevel*, const FPlacedPoint*>* Arrival = ArrivalById.Find(DesignPortal.Key);
        if (Arrival == nullptr)
        {
            Error(FString::Printf(TEXT("포털 %d의 도착 지점이 없다. 도착 지역 %d의 맵 레벨에 도착 지점을 둔다"), DesignPortal.Key, DesignPortal.Value.DestAreaId));
            continue;
        }

        const FString& LevelName = Arrival->Key->LevelName;
        const FPlacedPoint& Point = *Arrival->Value;
        const TPair<const FLevel*, const FPlacedArea*>* DestArea = AreaById.Find(DesignPortal.Value.DestAreaId);
        if (DestArea == nullptr || DestArea->Key != Arrival->Key)
            Error(FString::Printf(TEXT("%s %s: 포털 %d의 도착 지역 %d이 이 레벨에 없다"), *LevelName, *Point.Name, DesignPortal.Key, DesignPortal.Value.DestAreaId));
        else if (ContainsPoint(DestArea->Value->Box, Point.Location) == false)
            Error(FString::Printf(TEXT("%s %s: 포털 %d의 도착 지점이 도착 지역 %d 밖에 있다"), *LevelName, *Point.Name, DesignPortal.Key, DesignPortal.Value.DestAreaId));
    }

    if (Errors.IsEmpty() == false)
        return Result;

    // 3) 산출물. 행은 번호 순서다.
    for (FMapRows& Rows : MapRows)
    {
        Rows.Areas.Sort([](const FPlacedArea& A, const FPlacedArea& B) { return A.AreaId < B.AreaId; });
        Rows.RespawnPoints.Sort([](const TPair<int32, const FPlacedPoint*>& A, const TPair<int32, const FPlacedPoint*>& B) { return A.Key < B.Key; });
        Rows.Portals.Sort([](const FPlacedPoint& A, const FPlacedPoint& B) { return A.Id < B.Id; });

        TArray<TPair<const FPlacedPoint*, const FPlacedPoint*>> Portals;
        for (const FPlacedPoint* Portal : Rows.Portals)
            Portals.Add({ Portal, ArrivalById[Portal->Id].Value });

        Result.Files.Add(Rows.Level->MapId, WriteFile(*Rows.Level, *Rows.Map, Rows.Areas, Rows.RespawnPoints, Portals));
    }

    return Result;
}
