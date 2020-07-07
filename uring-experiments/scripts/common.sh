devname() {
  echo $(sudo smartctl -i $1 | grep 'Model Number:' | sed 's/Model Number://')
}

validatedevs() {
  for dev in "${DEVS[@]}"; do
    devname="$(devname $dev)"
    echo $dev 
    if [ "$devname" == "" ]; then
      echo "WARNING: $dev doesn't seem to have correct permissions"
    else
      echo "Using $dev $devname"
    fi
  done
}

mklogdir() {
  expname="$1"
  dir="$expname-$(date +%Y%m%d%H%M%S)"
  fulldir="results/$dir"
  mkdir -p "$fulldir"
  cd results
  rm latest 2> /dev/null || true
  ln -s "$dir" latest
  cd - > /dev/null
  echo $fulldir
}

sync_results() {
  pushd results
  git pull
  git add *
  git commit -m 'sync_results'
  git push
  popd > /dev/null
}

