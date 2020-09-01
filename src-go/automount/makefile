build:
	@go build -o trident-automount

clean:
	@rm trident-automount

install:
	@cp trident-automount ${DESTDIR}/usr/bin/.
	mkdir -p ${DESTDIR}/etc/sv/trident-automount
	cp sv/run ${DESTDIR}/etc/sv/trident-automount/run
