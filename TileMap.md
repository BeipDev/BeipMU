Tilemaps let you encode any single layer 2D tile map in a compact JSON format.

# beip.tilemap

```
beip.tilemap.info {
  "Map1":
  {
     "tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/Ultima5.png",
     "tile-size":"16,16",
     "map-size":"10,4",
     "encoding":"Hex_4"
  }
}
beip.tilemap.data { "Map1":"0123456701234567012345670123456701234567" }
```

![Image of Map1](/images/TileMap1.png)

## beip.tilemap.info

This message will cause the map window to appear and describes all of the properties of it. It is needed before the data message is sent. If a map already exists and a new info message is received, it will change any properties that change (new tileset, etc..) if the map size is modified then the map data is initialized to tile '0' of the new size.

## beip.tilemap.data

This message holds the map data itself, and must match what the info message has described (more/less data will result in an error). Note that any number of data messages can be sent without a new info message. This makes it easier to update map content when nothing else changes.

### JSON Structure

```
beip.tilemap.info {
  "<map name>":
  {
    "tile-url":"<URL to download tile graphics from>",
    "tile-size":"<size of tiles in the tile image>",
    "map-size":"<size of the map in tiles>",
    "encoding":"<format of the map data>"
  }
  <repeats optionally for more windows...>
}

beip.tilemap.data {
  "<map name>":"<string of map data, content depends on encoding>"
  <repeats optionally for more windows...>
}
```

**map name** - The name to show in the map window, also how maps are identified by the GMCP messages

**tile-url "url string"** - The URL of the tilemap file, can be any size and format
* The tiles are assumed to be stored left to right, top to bottom without gaps. There is no restriction on the aspect ratio of the tilemap. If a tile map holds fewer tiles than are referenced by the map data, the wrong graphics might be drawn (or none at all). If a tile map holds more tiles, then it's just being wasteful.

**tile-size "width,height"** - The size in pixels of the tiles in the tilemap
* tile-size is currently restricted to at most 256 in each dimension ("tile-size":"256,256") most tiles are probably 32 by 32 or 16 by 16.

**map-size "width,height"** - The size of the map in tiles
* map-size is currently restricted to a maximum size of 256 by 256 ("map-size":"256,256"). That is a huge maximum and most maps are expected to be much smaller, like "map-size":"32,32".

**encoding "string"** - Encoding format of the content data (the 012345... part between the tags)
 encoding has multiple possibilities to make it easy to use or as compact as possible:

* hex_4 - Hexadecimal with 4 bits per tile (16 tiles possible). In this format a single hex character is a single tile.
** The tilemap tag at the start of this document uses hex_4 encoding
* hex_8 - Same as hex_4 but it's two hex digits (256 tiles possible)
* base64_8 - Base64 encoded 8 bits per tile
* zbase64_8 - The data is zlib compressed then Base64 encoded, this is great for static content that doesn't change.

The map data itself is just a binary block of data with the ordering being from left to right, then top to bottom.

## Tileset example

![Image of the tileset](/images/Ultima5.png)

## Map examples

```
beip.tilemap.info { "Lighthouse 1":{ "tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/Ultima5.png", "tile-size":"16,16", "map-size":"32,32", "encoding":"zbase64_8" }}
beip.tilemap.data { "Lighthouse 1": "tZBBCsIwEEVXuc5g9Qqz9h9AvVNRcS0URGh13YXo1mM5bRI6xsmiWj80/eHNHz5xbjqRZWnq3fSfxqPYSMErtxdgBk5q5gPzs5+5QStywdwPLblabzrzwIXPmodEKbblcCQ8IjFbRt0YXL7rDry/H5DkvcSV4aibFXR/3xGt0T8OAIVcFvb7dUGX4xH8xGcixUn/i2HUzs+9cvy9Rk5Hk1Oap2/3W/wF" }
```

![Image of Lighthouse](/images/LightHouse.png)

```
beip.tilemap.info { "Laboratory":{ "tile-url":"https://github.com/BeipDev/BeipMU/raw/master/images/FutureTiles.png", "tile-size":"32,32", "map-size":"16,16", "encoding":"zbase64_8" }}
beip.tilemap.data { "Laboratory": "k+ZFAXzaLKxIgE1/5ixHhtsMYBBzLFlMf+WqQIbXrxg4OLkYcq4Vi+lrg2R+MUhISjHUMDSA+FqLQULGIALCnw7RvnsnlL+cQRfEP30Syg9lsGW4fJmB4SZcngHIvwzXv50BLMBwB8yPZUgHG3eZ4Q2YX8vQzhDNcIQBZt9XBgQA8VVUkYCavo8vCvADAA" }
```

![Image of Laboratory](/images/Laboratory.png)

## Usage in BeipMU

To enable tilemap tag parsing, type: /tilemap on

To show an example tilemap, use: /test tilemap1

