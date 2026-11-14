extends Sprite2D
## A weapon pickup. Walking over it equips its skin on the player.

@export var weapon := &"red"

var cooldown := 0.0
