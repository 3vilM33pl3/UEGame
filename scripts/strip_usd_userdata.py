import unreal

ROOTS = [
    "/Game/CanalMarket",
]

USD_CLASS_MARKERS = [
    "Usd",
    "USD",
    "USDClasses",
]

registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = []
for root in ROOTS:
    assets += registry.get_assets_by_path(root, recursive=True)

removed_total = 0
changed = 0

for asset_data in assets:
    asset = asset_data.get_asset()
    if asset is None:
        continue

    # Not all assets have asset_user_data
    try:
        user_data = list(asset.get_editor_property("asset_user_data"))
    except Exception:
        continue

    if not user_data:
        continue

    kept = []
    removed = []
    for ud in user_data:
        try:
            cls = ud.get_class()
            cls_name = cls.get_name() if cls else ""
            cls_path = cls.get_path_name() if cls else ""
        except Exception:
            cls_name = ""
            cls_path = ""

        marker_hit = any(m in cls_name or m in cls_path for m in USD_CLASS_MARKERS)
        if marker_hit:
            removed.append(ud)
        else:
            kept.append(ud)

    if removed:
        asset.set_editor_property("asset_user_data", kept)
        removed_total += len(removed)
        changed += 1
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True)

print(f"USD user data removed from {changed} assets, {removed_total} entries removed.")
