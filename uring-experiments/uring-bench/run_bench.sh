trap 'kill %1; kill %2; kill %3' SIGINT
./bench -p & ./bench -p & ./bench -p & ./bench -p
