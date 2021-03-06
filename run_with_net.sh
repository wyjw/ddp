# Make sure have e1000 config kernel
# brctl addbr br0
# brctl addif br0 eth0 (or whatever it is named on host)
# when in client, dhclient

sudo qemu-system-x86_64 -curses -kernel ../kernel/linux/arch/x86_64/boot/bzImage -m 4096 -smp 4 -hda qemu-image.img -append "root=/dev/sda rw console=ttyS0" -device e1000,netdev=net0,mac=DE:AD:BE:EF:AA:BB -netdev tap,id=net0 -device e1000,netdev=net1 -netdev user,id=net1,hostfwd=tcp::5555-:22 -boot c -drive file=nvme.img,if=none,id=D22 -device nvme,drive=D22,serial=1234 -serial file:a.out
