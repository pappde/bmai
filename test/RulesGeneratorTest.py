#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: Copyright © 2026 Denis Papp <denis@accessdenied.net>

import importlib.util
import pathlib
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "rules_generator", ROOT / "scripts" / "update_buttonweavers_skills.py"
)
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)


def fixture():
    return {
        "source": "https://example.test/rules",
        "fetched_at": "2026-08-10T00:00:00Z",
        "api_requests": [],
        "dieSkills": {
            "k": {
                "name": "Konstant",
                "description": "Upstream prose",
                "interacts": {"Trip": "More upstream prose"},
            }
        },
        "dieTypes": {"s": {"name": "Swing", "description": "Upstream prose"}},
    }


class RulesGeneratorTest(unittest.TestCase):
    def test_normalize_keeps_only_identifiers(self):
        snapshot = GENERATOR.normalize(fixture())

        self.assertEqual({"name": "Konstant"}, snapshot["dieSkills"]["k"])
        self.assertEqual({"name": "Swing"}, snapshot["dieTypes"]["s"])

    def test_normalize_rejects_incomplete_entries(self):
        candidate = fixture()
        candidate["dieSkills"]["k"] = {}

        with self.assertRaisesRegex(ValueError, "has no name"):
            GENERATOR.normalize(candidate)

    def test_render_links_without_copying_upstream_prose(self):
        rendered = GENERATOR.render(GENERATOR.normalize(fixture()))

        self.assertIn("Konstant", rendered)
        self.assertIn("https://example.test/rules", rendered)
        self.assertNotIn("Upstream prose", rendered)

    def test_local_reference_includes_upstream_prose(self):
        rendered = GENERATOR.render_local(fixture())

        self.assertIn("Upstream prose", rendered)
        self.assertIn("not distributed with BMAI", rendered)

    def test_write_failure_preserves_existing_outputs(self):
        snapshot = GENERATOR.normalize(fixture())
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            snapshot_path = root / "missing" / "rules.json"
            reference_path = root / "rules.md"
            reference_path.write_text("known good", encoding="utf-8")

            with self.assertRaises(FileNotFoundError):
                GENERATOR.write_outputs(snapshot, GENERATOR.render(snapshot), snapshot_path, reference_path)
            self.assertEqual("known good", reference_path.read_text(encoding="utf-8"))

    def test_second_replace_failure_restores_snapshot(self):
        snapshot = GENERATOR.normalize(fixture())
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            snapshot_path = root / "rules.json"
            reference_path = root / "rules.md"
            snapshot_path.write_text("known snapshot", encoding="utf-8")
            reference_path.write_text("known reference", encoding="utf-8")
            replacements = 0

            def fail_second(source, destination):
                nonlocal replacements
                replacements += 1
                if replacements == 2:
                    raise PermissionError("read only")
                source.replace(destination)

            with self.assertRaises(PermissionError):
                GENERATOR.write_outputs(
                    snapshot, GENERATOR.render(snapshot), snapshot_path, reference_path, fail_second
                )
            self.assertEqual("known snapshot", snapshot_path.read_text(encoding="utf-8"))
            self.assertEqual("known reference", reference_path.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
