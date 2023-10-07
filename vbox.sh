VM_NAME="OS"
FLOPPY="bin/floppy.img"

cp -f bin/floppy.bin $FLOPPY
VBoxManage unregistervm $VM_NAME --delete 
VBoxManage createvm --name $VM_NAME --register
VBoxManage modifyvm $VM_NAME --ioapic off
VBoxManage storagectl $VM_NAME --name "Floppy Controller" --add floppy
VBoxManage storageattach $VM_NAME --storagectl "Floppy Controller" --port 0 --device 0 --type fdd --medium $FLOPPY
VBoxManage startvm $VM_NAME