"""Keep the docs homepage header list synchronized with README.md."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
DOCS_INDEX = ROOT / "docs" / "index.md"


def main() -> None:
    readme = README.read_text(encoding="utf-8")
    start = readme.index("### Common headers")
    end = readme.index("The common files own", start)
    inventory = readme[start:end].strip()

    index = DOCS_INDEX.read_text(encoding="utf-8")
    marker = "## Header map"
    index_start = index.index(marker)
    DOCS_INDEX.write_text(
        index[:index_start] + marker + "\n\n" + inventory + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
