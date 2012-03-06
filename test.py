#-----------------------------
#Python example code:
#-----------------------------

import liblvm

# create a new LVM instance 
lvm = liblvm.Liblvm()

# open a VG handle in write mode for 'myvg'
vg = lvm.vgOpen('myvg','w')

# remove a new LV (lv_foobar)
lv = vg.createLvLinear('lv_foobar',100000)

# print the uuid for the newly created LV
print lv.getUuid()

#print the size.
print lv.getSize()

# Add a tag to it.
lv.addTag("my_fance_tag")

# Print all tags.
print lv.getTags()

# Try to deactivate 
try:
	lv.deactivate()
except liblvm.LibLVMError:
	print "lets retry.."
	lv.deactivate()

# remove 
lv.remove()

# close VG handle
vg.close()

# close LVM handle
lvm.close()


