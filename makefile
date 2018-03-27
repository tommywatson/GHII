#
## MWLibs makefile
#

all: clean

clean: 
	find ./ -name gmon.out | xargs rm -f
	find ./ -name vgcore\* | xargs rm -f

commit: clean
	git add . 
	git commit
	git push

dollars:
	sloccount . 

