# This file is included by other scripts for config parameters like
# where the device is that should be tested.

declare -a DEVS
declare -A SPDKDEVS

./scripts/spdk-disable

count=0
for x in /dev/nvme{0,1}n1; do
  if [[ "$(sudo smartctl -x $x | grep SSDPED1D280GA)" != "" ]]; then
    DEVS+=( "$x" )
    SPDKDEVS[$x]='trtype=PCIe traddr=0000.02.00.0 ns=1'
    sudo chmod ugo+rw "$x" || true
    count=$((count + 1))
  fi
done

if [[ "$count" == "0" ]]; then
  echo ERROR - no valid devices populated into DEVS
  exit -1
fi

source ./scripts/common.sh
validatedevs
