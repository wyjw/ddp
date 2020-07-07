BEGIN {
  print "engine;dev;iodepth;iops;50th;99th"
}


/== / {
  engine=$3
  dev=$5
  iodepth=$7
}

/^Total/ {
  iops=$3
}

/ 50.00000% :/ {
  median=$3
  median=substr(median, 1, length(median)-2) # Remove "us" from end
}

/ 99.99000% :/ {
  tail=$3
  tail=substr(tail, 1, length(tail)-2) # Remove "us" from end

  print engine ";" dev ";" iodepth ";" iops ";" median ";" tail
}
