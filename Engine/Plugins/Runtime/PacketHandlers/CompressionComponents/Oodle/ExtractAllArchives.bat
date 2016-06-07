@echo off

echo.
echo This batch file will unzip all downloaded packet captures from S3.
echo.

set /p Directory=Type the root directory of the zipped captures: 
echo.

aws s3 sync s3://dedicated-server-ucap %Directory%

pushd %CD%
cd %Directory%
FOR /D /r %%F in ("*") DO (
	pushd %CD%
	cd %%F
		FOR %%X in (*.gz) DO (
			"C:\Program Files\7-zip\7z.exe" x "%%X" -y
		)
	popd
)
popd