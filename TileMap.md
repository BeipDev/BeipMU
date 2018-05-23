# TileMap Tags

Tilemaps let you encode any single layer 2D tile map in a compact and simple XML based format.

```
<tilemap name='Map1' tiles='http://wiki.ultimacodex.com/images/5/55/Ultima_5_-_Tiles-pc.png' tileSize='16,16' mapSize='10,4' enc='hex_4'>0123456701234567012345670123456701234567</tilemap>
```

![Image of Map1](/images/TileMap1.png)

## Tag Attributes

**name='string'** - The name to show in the map window

**tiles='string'** - The URL of the tilemap file, can be any size and format
* The tiles are assumed to be stored left to right, top to bottom without gaps. There is no restriction on the aspect ratio of the tilemap. If a tile map holds fewer tiles than are referenced by the map data, the wrong graphics might be drawn (or none at all). If a tile map holds more tiles, then it's just being wasteful.

**tileSize='width,height'** - The size in pixels of the tiles in the tilemap
* tileSize is currently restricted to at most 256 in each dimension (tileSize='256,256') most tiles are probably 32 by 32

**mapSize='width,height'** - The size of the map in tiles
* mapSize is currently restricted to 256 by 256 (mapSize='256,256')

**enc='string'** - Encoding format of the content data (the 012345... part between the tags)
 enc has multiple possibilities to make it easy to use or as compact as possible:

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
<tilemap name='Lighthouse' tiles='http://wiki.ultimacodex.com/images/5/55/Ultima_5_-_Tiles-pc.png' tileSize='16,16' mapSize='32,32' enc='zbase64_8'>eJytkkEKwjAQRVe5TrB6hVn7D6DeqWhxXRBEaHXdhejWYzltEjrWGbpoPjT54f2ZDiHO5ZPXrFeCi3pnajjbK+N/EGT1BYiAq8j8YfoMmSekEmdMQ2hLl/2hN2/c6SZ5rCjZdhSXCU+IzZHQtArn73ECVa8zJvVB7Mq4NO0Ocv4wIzpl/hQACj5s9PvrC53FE1jEVyzBvdyLMarXr4Ms/juGpVrl4r0Ebj/Mmf4a/wJ4Ll9p</tilemap>
```

![Image of Lighthouse](/images/LightHouse.png)
