extends Node3D
## Small and distant characters, without (left) and with (right) mipmaps.

func _ready() -> void:
	var plain := SpriteLoopData.load_from_file("res://tests/fixtures/clockheart_gun.spla")
	var mipmapped := SpriteLoopData.load_from_file("res://tests/fixtures/clockheart_gun.spla", true)
	for column in 2:
		var data := mipmapped if column == 1 else plain
		for i in 4:
			# 2D: scaled down to about 8% (about 40 px tall).
			var flat := SpriteLoop2D.new()
			flat.data = data
			flat.skin = &"red"
			flat.position = Vector2(80 + column * 480 + i * 100, 140)
			flat.scale = Vector2(0.08, 0.08)
			flat.frame = i * 5
			if column == 1:
				flat.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
			$Canvas.add_child(flat)
			# 3D: far from the camera.
			var spatial := SpriteLoop3D.new()
			spatial.data = data
			spatial.skin = &"red"
			spatial.position = Vector3(-26.0 + column * 30.0 + i * 6.0, -3.0, -90)
			spatial.frame = i * 5
			spatial.texture_filter = (SpriteLoop3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS if column == 1
					else SpriteLoop3D.TEXTURE_FILTER_LINEAR)
			add_child(spatial)
