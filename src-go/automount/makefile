build:
	@go build -o trident-automount

clean:
	@rm trident-automount

install:
	@cp trident-automount /usr/bin/.
	mkdir -p /etc/sv/trident-automount
	cp sv/run /etc/sv/trident-automount/run
