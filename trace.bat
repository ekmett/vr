@echo off

pushd d:\vr

del *.trace

apitrace trace bin\win64\proc.exe

qapitrace proc.trace

popd
