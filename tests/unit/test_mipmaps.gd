extends "res://tests/test_case.gd"
## Atlas mipmaps: the "mipmaps" option on SpriteLoopData and the 3D filter.


## Atlas payload created by loading the package (load_from_file builds it
## while validating). The data stays alive in `keep` while measuring.
func atlas_bytes(mipmaps: bool, keep: Array) -> int:
	var before: int = SpriteLoopServer.get_stats().atlas_bytes
	var data := SpriteLoopData.load_from_file(TURRET, mipmaps)
	keep.append(data)
	check_eq(data.has_mipmaps(), mipmaps, "mipmaps option")
	return SpriteLoopServer.get_stats().atlas_bytes - before


func test_mipmapped_atlas_costs_about_a_third_more() -> void:
	var keep := []
	var plain_bytes := atlas_bytes(false, keep)
	var mip_bytes := atlas_bytes(true, keep)
	check(plain_bytes > 0, "atlas created")
	# Mip levels add 1/4 + 1/16 + ... = 1/3; wider padding adds a little more.
	var ratio := float(mip_bytes) / maxi(plain_bytes, 1)
	check(ratio > 1.3 and ratio < 1.6, "mip levels add about a third (%.2f)" % ratio)


func test_toggling_mipmaps_reloads_nodes() -> void:
	var data := SpriteLoopData.load_from_file(TURRET)
	var node := make_2d(TURRET)
	node.data = data
	node.play(&"shoot")
	node.advance(0.2)
	var frame := node.frame
	var before: int = SpriteLoopServer.get_stats().atlas_bytes
	data.mipmaps = true
	check(SpriteLoopServer.get_stats().atlas_bytes > before, "atlas rebuilt with mipmaps")
	check(node.is_playing(), "playback kept")
	check_eq(node.frame, frame, "frame kept")


func test_3d_mipmap_filter() -> void:
	var node := SpriteLoop3D.new()
	_owned.append(node)
	node.data = SpriteLoopData.load_from_file(TURRET, true)
	node.texture_filter = SpriteLoop3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
	check_eq(node.texture_filter, SpriteLoop3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, "filter set")
	node.texture_filter = SpriteLoop3D.TEXTURE_FILTER_NEAREST
	check_eq(node.texture_filter, SpriteLoop3D.TEXTURE_FILTER_NEAREST, "back to nearest")
