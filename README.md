# SimpleHTML
### A simple indent-based markup language that converts into HTML, XML and such

# Syntax
```
head
    title
        "4 spaces of tab!"
body
    h1
        'Multiple lines work too.
        This code adds <br/> at newlines.
        \' and " are functionally equvalent.'
    /br
    # This is a comment to say that self-closing
    # tags need a slash in front of them.

    script
        `alert("A backtick (\`) is similar to \\" and '");
        alert("except that instead of <br> the newlines are simply newlines")`
```
This is converted to
```html
<head><title>4 spaces of tab!</title></head><body><h1>Multiple lines work too.<br/>This code adds <br/> at newlines.<br/>' and " are functionally equvalent.</h1><br/><!-- This is a comment to say that self-closing--><!-- tags need a slash in front of them.--><script>alert("A backtick (`) is similar to \" and '");
alert("except that instead of <br> the newlines are simply newlines")</script></body>
```
