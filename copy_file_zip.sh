rm ./tools/DownloadTool/H9G_CONFIG/mf/app/*.so  ./tools/DownloadTool/H9G_CONFIG/mf/app/*.bin
cp ./SDK/out/LINUX_320_240/*.bin ./tools/DownloadTool/H9G_CONFIG/mf/app
cp ./SDK/out/LINUX_320_240/*.so ./tools/DownloadTool/H9G_CONFIG/mf/app
cd ./tools/DownloadTool/H9G_CONFIG
mv  mf/app/demo.bin  mf/app/dev.bin
rm  mf.zip
zip -r mf.zip mf/*
