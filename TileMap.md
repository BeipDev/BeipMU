# TileMap Tags

Tilemaps let you encode any single layer 2D tile map in a compact and simple format.

<tilemap name='Map1' tiles='http://wiki.ultimacodex.com/images/5/55/Ultima_5_-_Tiles-pc.png' tileSize='16,16' mapSize='10,4' enc='hex_4'>0123456701234567012345670123456701234567</tilemap>

![Image of Map1](images\TileMap1.png)

The simple explanation of the tags:

name - The name to show in the map window
tiles - The URL of the tilemap file, can be any size and format
tileSize - The size in pixels of the tiles
mapSize - The size of the map in tiles
enc - Encoding format of the content data (the 012345... part between the tags)

tileSize is currently restricted to at most 256 in each dimension (tileSize='256,256') most tiles are probably 32 by 32
mapSize is currently restricted to 256 by 256 (mapSize='256,256')

<tilemap name='Lighthouse' tiles='http://wiki.ultimacodex.com/images/5/55/Ultima_5_-_Tiles-pc.png' tileSize='16,16' mapSize='32,32' enc='zbase64_8'>eJytkkEKwjAQRVe5TrB6hVn7D6DeqWhxXRBEaHXdhejWYzltEjrWGbpoPjT54f2ZDiHO5ZPXrFeCi3pnajjbK+N/EGT1BYiAq8j8YfoMmSekEmdMQ2hLl/2hN2/c6SZ5rCjZdhSXCU+IzZHQtArn73ECVa8zJvVB7Mq4NO0Ocv4wIzpl/hQACj5s9PvrC53FE1jEVyzBvdyLMarXr4Ms/juGpVrl4r0Ebj/Mmf4a/wJ4Ll9p</tilemap>

![Image of Lighthouse](images\LightHouse.png)
