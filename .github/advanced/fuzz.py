"""Fuzz public-source publication gate, without disc images or compiled game."""
from hypothesis import given, settings, strategies as st
from scripts import check_repository as hygiene

CI = settings(max_examples=180, deadline=None, derandomize=True)

@CI
@given(st.text(max_size=110))
def fuzz_public_path(path):
    allowed = hygiene.allowed(path)
    assert isinstance(allowed, bool)
    if allowed:
        assert not path.startswith("/")
        assert "\\" not in path
        assert ".." not in path.split("/")

@CI
@given(st.text(max_size=256))
def fuzz_source_inspection(source):
    findings = hygiene.inspect("docs/stage3-synthetic.md", source.encode("utf-8", "surrogatepass"))
    assert isinstance(findings, list)
    assert all(isinstance(problem, str) for problem in findings)

@CI
@given(st.text(alphabet="abcdefghijklmnopqrstuvwxyz0123456789_-", min_size=1, max_size=48))
def fuzz_traversal_segments(segment):
    assert not hygiene.allowed(f"docs/../{segment}.md")
    assert not hygiene.allowed(f"/{segment}.md")

if __name__ == "__main__":
    fuzz_public_path()
    fuzz_source_inspection()
    fuzz_traversal_segments()
    print("PASS: Saturn source-only publication properties")
