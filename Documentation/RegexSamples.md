# Popular regexes

## Channels

    [public] Text
    [chat] Text
    [OOC] Text

Regex: `^\[([^\]]+)\]`with the parameter being the name within the []. So for [public] \1 = public

If you only want specific channels to match, just list them directly: `^(\[Info\]|\[Public\]|\[etc\])`
