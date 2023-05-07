for f in `find . \( -name Makefile -o -name '*.c' -o -name '*.h' -o -name '*.sh' -o -name '*.xml' -o -name '*.html' \)`;do
	chmod +w $f
	dos2unix $f
done
