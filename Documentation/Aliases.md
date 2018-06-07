Aliases transform what you send to the server. For example, if you wanted to turn 'Xyl' into 'Xylophone' you could have an alias with Match Text of 'Xyl' and have it be an alias for 'Xylophone'.

Aliases can also do regular expression matching with replacement. So for example, if you want to turn this:

test alpha-beta

into:

I just walked to alpha and said hello to beta

You can use 'test(alpha)-(beta)' as the Match Text, and 'I just walked to \1 and said hello to \2' as the replacement.

Works with regex if expressions nicely too:

Match Text:test (alpha|beta|gamma)-(alpha|beta|gamma)
Replacement:"Testing a send of \1 as the first match parameter and \2 as the second

Note that this isn't a BeipMU feature, it's a feature of regular expressions!
