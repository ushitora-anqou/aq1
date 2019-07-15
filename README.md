# aq1

Yet another tiny tiny calculator.

## `0.11762 - 0.13172 + 0.01410 = ?`

```
$ python
Python 3.7.3 (default, May 11 2019, 19:18:58)
[GCC 8.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> 0.11762 - 0.13172 + 0.01410
-1.734723475976807e-18

$ irb
irb(main):001:0> 0.11762 - 0.13172 + 0.01410
=> -1.734723475976807e-18

$ bc
bc 1.07.1
Copyright 1991-1994, 1997, 1998, 2000, 2004, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
This is free software with ABSOLUTELY NO WARRANTY.
For details type `warranty'.
0.11762 - 0.13172 + 0.01410
0

$ aq1
>> 0.11762 - 0.13172 + 0.01410
0
```

## `1/3 + 1/3 + 1/3 = ?`

```
$ python
Python 3.7.3 (default, May 11 2019, 19:18:58)
[GCC 8.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> 1/3 + 1/3 + 1/3
1.0

$ irb
irb(main):001:0> 1/3 + 1/3 + 1/3
=> 0

$ bc
bc 1.07.1
Copyright 1991-1994, 1997, 1998, 2000, 2004, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
This is free software with ABSOLUTELY NO WARRANTY.
For details type `warranty'.
1/3 + 1/3 + 1/3
0

$ ./aq1
>> 1/3 + 1/3 + 1/3
1
```

## License

MIT License.
