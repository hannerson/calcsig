##########
# 20190113
##########

exclude_dirs=include bin jsoncpp glog test src
dirs := $(shell find . -maxdepth 1 -type d)
dirs := $(basename $(patsubst ./%,%,$(dirs)))
dirs := $(filter-out $(exclude_dirs),$(dirs))

AR = ar
ARFLAGS = -crs

cur_dir=$(shell pwd)
all_files=$(wildcard *)
subdirs:=$(dirs) src

all:
	@for n in $(subdirs);do $(MAKE) -C $$n; done


.PHONY clean:
clean:
	@for n in $(subdirs);do $(MAKE) clean -C $$n; done
