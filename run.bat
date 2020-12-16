@echo off
y:
cd \dev\bmai
echo test >> test.log
time /t >> test.log
c:\bin\perl\bin\perl.exe -w -S bmai_run.pl
