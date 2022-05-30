The copyit.c can copy files with the command:
```sh
copyit SourceFile TargetFile
```

copyit_extracredit.c can do a recursive copy on the directories using a similar format

##Testing
```sh
% md5sum /tmp/SourceFile
b92891465b9617ae76dfff2f1096fc97  /tmp/SourceFile
% md5sum /tmp/TargetFile
b92891465b9617ae76dfff2f1096fc97  /tmp/TargetFile
```
