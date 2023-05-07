find . -type d | xargs chmod 755
find . -type f | xargs chmod 644
find . -name '*.sh' | xargs chmod 755
chmod 755 sqlite
find .. \( -name Makefile -o -name '*.sh' -o -name '*.c' -o -name '*.h' \) | xargs touch
