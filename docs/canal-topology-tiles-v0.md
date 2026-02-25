# Canal Topology Tiles v0 (M1)

This is the reference for `FCanalPrototypeTileSet::BuildV0()` and the matching
Data Asset authoring workflow.

## Direction Legend

Socket order is `[E, NE, NW, W, SW, SE]`.

Shorthand:
- `W` = Water
- `B` = Bank
- `TL` = TowpathL
- `TR` = TowpathR
- `L` = Lock
- `R` = Road

Mini diagram template:

```text
         NE(1)   NW(2)
      E(0)  [hex]  W(3)
         SE(5)   SW(4)
```

## Tile List (13)

Required category coverage:
- straight: `water_straight_*`
- gentle bend: `water_bend_gentle`
- hard bend: `water_bend_hard`
- junction: `water_t_junction`
- lock boundary: `lock_gate`
- optional dead-end: `water_dead_end`

### 1. water_straight_ew
- Sockets: `[W, B, B, W, B, B]`
- Weight: `1.0`
- Diagram: east-west through channel.

### 2. water_straight_nesw
- Sockets: `[B, W, B, B, W, B]`
- Weight: `1.0`
- Diagram: northeast-southwest through channel.

### 3. water_straight_nwse
- Sockets: `[B, B, W, B, B, W]`
- Weight: `1.0`
- Diagram: northwest-southeast through channel.

### 4. water_bend_gentle
- Sockets: `[W, W, B, B, B, B]`
- Weight: `0.9`
- Diagram: gentle corner with two adjacent water edges.

### 5. water_bend_hard
- Sockets: `[W, B, W, B, B, B]`
- Weight: `0.7`
- Diagram: harder corner with one bank gap between water edges.

### 6. water_t_junction
- Sockets: `[W, W, B, W, B, B]`
- Weight: `0.35`
- Diagram: three-way split.

### 7. water_cross
- Sockets: `[W, B, W, W, B, W]`
- Weight: `0.15`
- Diagram: dense crossing variant.

### 8. water_dead_end
- Sockets: `[W, B, B, B, B, B]`
- Weight: `0.2`
- Boundary port: `true`
- Diagram: one-sided water terminator.

### 9. lock_gate
- Sockets: `[L, B, B, L, B, B]`
- Weight: `0.25`
- Boundary port: `true`
- Diagram: lock segment across opposite edges.

### 10. towpath_left
- Sockets: `[W, TL, B, W, TL, B]`
- Weight: `0.5`
- Diagram: water pair with left towpath semantics.

### 11. towpath_right
- Sockets: `[W, B, TR, W, B, TR]`
- Weight: `0.5`
- Diagram: water pair with right towpath semantics.

### 12. road_crossing
- Sockets: `[R, B, W, R, B, W]`
- Weight: `0.2`
- Diagram: road-water crossing prototype.

### 13. solid_bank
- Sockets: `[B, B, B, B, B, B]`
- Weight: `0.6`
- Diagram: filler / non-water area.

## Data Asset Authoring Workflow

1. Create a `CanalTopologyTileSetAsset` in editor.
2. Use **Call In Editor** function `PopulateWithPrototypeV0`.
3. Validate with `BuildCompatibilityCache` and `DescribeCompatibility`.

Reference APIs:
- `Source/UEGame/Public/CanalGen/CanalTopologyTileSetAsset.h`
- `Source/UEGame/Private/CanalGen/CanalTopologyTileSetAsset.cpp`
