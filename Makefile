.PHONY: docs

docs:
	uv run --with-requirements docs/requirements.txt python docs/sync_header_inventory.py
	uv run --with-requirements docs/requirements.txt sphinx-build --keep-going -b html docs docs/_build/html
