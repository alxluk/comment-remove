CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -O2
TARGET = comment-remove
SRC = comment-remove.c

.PHONY: all check-diff clean

all: check-diff $(TARGET)

check-diff:
	@command -v diff >/dev/null 2>&1 || { printf "Error: diffutils required. Install with:\n  sudo apt install diffutils    # Debian/Ubuntu\n  sudo yum install diffutils    # RHEL/CentOS\n  sudo pacman -S diffutils      # Arch/Manjaro\n"; exit 1; }

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
