ioup
====

pub.iotek.org cli uploader

###examples
ioup can be used in many different ways,


you can upload files,
```sh
ioup $HOME/.xinitrc
```

pipe stuff into it,
```sh
df -h | ioup
```

you can remove uploaded files,
```sh
ioup -r p/FiLEnAmE
```

list all uploaded files.
```sh
ioup -l
```
Token is read from the shell variable `IOUP_TOKEN`

To remove all files associated with your token, you can do something like;
```sh
for file in `ioup -l | awk '{print $1}'`; do
  ioup -r $file
done
```
