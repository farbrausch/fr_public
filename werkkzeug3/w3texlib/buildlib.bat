@echo off
md Build_MT 2>nul
md Build_MTd 2>nul
md Build_MD 2>nul
md Build_MDd 2>nul
md Libs 2>nul
set OUTDIR=Libs
set OPTS=/nologo /EHs-c- /GS- /GR-
set DBGOPTS=/Od /Zi /D_DEBUG
set RELOPTS=/O2 /Gy /DNDEBUG
nmake %1 OUTDIR=Libs OUTFILE=w3texlib_vc80_MT.lib BUILDDIR=Build_MT CFLAGS="%OPTS% %RELOPTS% /MT" /nologo
nmake %1 OUTDIR=Libs OUTFILE=w3texlib_vc80_MTd.lib BUILDDIR=Build_MTd CFLAGS="%OPTS% %DBGOPTS% /MTd" /nologo
nmake %1 OUTDIR=Libs OUTFILE=w3texlib_vc80_MD.lib BUILDDIR=Build_MD CFLAGS="%OPTS% %RELOPTS% /MD" /nologo
nmake %1 OUTDIR=Libs OUTFILE=w3texlib_vc80_MDd.lib BUILDDIR=Build_MDd CFLAGS="%OPTS% %DBGOPTS% /MDd" /nologo