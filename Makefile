
all: default

default:
	gcc parse.c tracker.c -lpthread -o tracker

clean: 
	@rm -rf tracker
