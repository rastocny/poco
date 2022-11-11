@echo off
buildwin 160 build shared both x64 samples tests msbuild env

REM Foundation\include\Poco\Config.h - enable POCO_NEW_STATE_ON_MOVE, define #define POCO_EXTERNAL_OPENSSL 2 (same as POCO_EXTERNAL_OPENSSL_SLPRO in Crypto.h)
