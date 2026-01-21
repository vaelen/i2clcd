# Copyright (c) 2026 Andrew C. Young
# SPDX-License-Identifier: MIT
#
# Makefile for libi2clcd and lcdctl

# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -std=c99 -O2
CFLAGS  += -D_POSIX_C_SOURCE=199309L
LDFLAGS :=

# Debug build
ifdef DEBUG
CFLAGS  += -g -O0 -DDEBUG
endif

# Directories
SRCDIR   := src
INCDIR   := include
APPDIR   := app
EXDIR    := examples
BUILDDIR := build
LIBDIR   := $(BUILDDIR)/lib
BINDIR   := $(BUILDDIR)/bin
OBJDIR   := $(BUILDDIR)/obj

# Library name
LIBNAME   := i2clcd
LIBSTATIC := $(LIBDIR)/lib$(LIBNAME).a
LIBSHARED := $(LIBDIR)/lib$(LIBNAME).so

# Source files
LIB_SRCS := $(SRCDIR)/i2clcd.c
LIB_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))

APP_SRCS := $(APPDIR)/lcdctl.c
APP_OBJS := $(patsubst $(APPDIR)/%.c,$(OBJDIR)/%.o,$(APP_SRCS))
APP_BIN  := $(BINDIR)/lcdctl

DEMO_SRCS := $(EXDIR)/demo.c
DEMO_OBJS := $(patsubst $(EXDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRCS))
DEMO_BIN  := $(BINDIR)/demo

# Include paths
INCLUDES := -I$(INCDIR) -I$(SRCDIR)

#---------------------------------------------------------------------------
# Targets
#---------------------------------------------------------------------------

.PHONY: all lib app examples clean install uninstall help

all: lib app

lib: $(LIBSTATIC) $(LIBSHARED)

app: $(APP_BIN)

examples: $(DEMO_BIN)

#---------------------------------------------------------------------------
# Directory creation
#---------------------------------------------------------------------------

$(OBJDIR) $(LIBDIR) $(BINDIR):
	mkdir -p $@

#---------------------------------------------------------------------------
# Library build
#---------------------------------------------------------------------------

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c $< -o $@

$(LIBSTATIC): $(LIB_OBJS) | $(LIBDIR)
	ar rcs $@ $^

$(LIBSHARED): $(LIB_OBJS) | $(LIBDIR)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------------
# Application build
#---------------------------------------------------------------------------

$(OBJDIR)/lcdctl.o: $(APPDIR)/lcdctl.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(APP_BIN): $(OBJDIR)/lcdctl.o $(LIBSTATIC) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------------
# Examples build
#---------------------------------------------------------------------------

$(OBJDIR)/demo.o: $(EXDIR)/demo.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(DEMO_BIN): $(OBJDIR)/demo.o $(LIBSTATIC) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------------
# Install/Uninstall
#---------------------------------------------------------------------------

PREFIX ?= /usr/local

install: lib app
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 644 $(LIBSTATIC) $(DESTDIR)$(PREFIX)/lib/
	install -m 755 $(LIBSHARED) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(INCDIR)/i2clcd.h $(DESTDIR)$(PREFIX)/include/
	install -m 755 $(APP_BIN) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/lib$(LIBNAME).a
	rm -f $(DESTDIR)$(PREFIX)/lib/lib$(LIBNAME).so
	rm -f $(DESTDIR)$(PREFIX)/include/i2clcd.h
	rm -f $(DESTDIR)$(PREFIX)/bin/lcdctl

#---------------------------------------------------------------------------
# Clean
#---------------------------------------------------------------------------

clean:
	rm -rf $(BUILDDIR)

#---------------------------------------------------------------------------
# Help
#---------------------------------------------------------------------------

help:
	@echo "Targets:"
	@echo "  all       - Build library and application (default)"
	@echo "  lib       - Build static and shared library"
	@echo "  app       - Build lcdctl application"
	@echo "  examples  - Build example programs"
	@echo "  install   - Install to PREFIX (default: /usr/local)"
	@echo "  uninstall - Remove installed files"
	@echo "  clean     - Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1   - Enable debug build"
	@echo "  PREFIX=   - Installation prefix"
