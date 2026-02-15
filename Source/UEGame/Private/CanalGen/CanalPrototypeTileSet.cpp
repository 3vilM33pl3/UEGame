#include "CanalGen/CanalPrototypeTileSet.h"

namespace
{
	FCanalTopologyTileDefinition MakeTile(
		const FName TileId,
		const std::initializer_list<ECanalSocketType> Sockets,
		const float Weight,
		const bool bBoundaryPort = false)
	{
		FCanalTopologyTileDefinition Tile;
		Tile.TileId = TileId;
		Tile.Sockets = TArray<ECanalSocketType>(Sockets);
		Tile.Weight = Weight;
		Tile.bAllowAsBoundaryPort = bBoundaryPort;
		return Tile;
	}
}

TArray<FCanalTopologyTileDefinition> FCanalPrototypeTileSet::BuildV0()
{
	TArray<FCanalTopologyTileDefinition> Tiles;
	Tiles.Reserve(13);

	Tiles.Add(MakeTile(TEXT("water_straight_ew"), {ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank}, 1.0f));
	Tiles.Add(MakeTile(TEXT("water_straight_nesw"), {ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Bank}, 1.0f));
	Tiles.Add(MakeTile(TEXT("water_straight_nwse"), {ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Water}, 1.0f));

	Tiles.Add(MakeTile(TEXT("water_bend_gentle"), {ECanalSocketType::Water, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank}, 0.9f));
	Tiles.Add(MakeTile(TEXT("water_bend_hard"), {ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank}, 0.7f));
	Tiles.Add(MakeTile(TEXT("water_t_junction"), {ECanalSocketType::Water, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank}, 0.35f));
	Tiles.Add(MakeTile(TEXT("water_cross"), {ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Water}, 0.15f));
	Tiles.Add(MakeTile(TEXT("water_dead_end"), {ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank}, 0.2f, true));

	Tiles.Add(MakeTile(TEXT("lock_gate"), {ECanalSocketType::Lock, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Lock, ECanalSocketType::Bank, ECanalSocketType::Bank}, 0.25f, true));
	Tiles.Add(MakeTile(TEXT("towpath_left"), {ECanalSocketType::Water, ECanalSocketType::TowpathL, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::TowpathL, ECanalSocketType::Bank}, 0.5f));
	Tiles.Add(MakeTile(TEXT("towpath_right"), {ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::TowpathR, ECanalSocketType::Water, ECanalSocketType::Bank, ECanalSocketType::TowpathR}, 0.5f));
	Tiles.Add(MakeTile(TEXT("road_crossing"), {ECanalSocketType::Road, ECanalSocketType::Bank, ECanalSocketType::Water, ECanalSocketType::Road, ECanalSocketType::Bank, ECanalSocketType::Water}, 0.2f));
	Tiles.Add(MakeTile(TEXT("solid_bank"), {ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank, ECanalSocketType::Bank}, 0.6f));

	return Tiles;
}
