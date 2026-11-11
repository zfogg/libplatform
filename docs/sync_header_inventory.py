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
    index_end = index.index("## Debugging that can live in a library", index_start)
    suffix = index[index_end:]
    legacy_start = suffix.find("\n- `agent.h`")
    if legacy_start >= 0:
        suffix = suffix[:legacy_start]
    DOCS_INDEX.write_text(
        index[:index_start] + marker + "\n\n" + inventory + "\n\n" + suffix.lstrip("\n"),
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
