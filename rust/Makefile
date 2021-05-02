
all: debug

debug:
	cargo build

releas:
	cargo build --release

clean:
	rm -rf *.gcno *.gcda lcov.info cover

test:
	cargo test

# target/cov/report/index.html
coverage:
	cargo cov clean
	cargo cov test
	cargo cov report --gcno . --gcda .

# cover/index.html
coverage-html: coverage
	lcov -c -d . -o lcov.info --no-external
	genhtml lcov.info -o cover
