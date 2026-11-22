"""Source-level guards for engine hooks (the Geode runtime is not available here).

Run with: python3 -m unittest discover -s tests -p '*_test.py'
These complement the standalone C++ policy tests, not in-game activation tests.
"""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / 'src/main.cpp').read_text()


class OrbIsolationTest(unittest.TestCase):
    def test_ghost_ring_cleanup_precedes_snapshot_restore(self):
        restore = SOURCE.split('    void restoreObjects() {', 1)[1].split(
            '    // v2.91:', 1)[0]
        start_restore = restore.index('for (auto const& s : buf.snapshots)')
        for ghost in ('ghost1', 'ghost2'):
            self.assertLess(restore.index(f'buf.{ghost}->resetTouchedRings(true);'),
                            start_restore)
        self.assertLess(start_restore, restore.index('buf.snapshots.clear();'))

    def test_snapshot_before_engine_collision_handling(self):
        collision = SOURCE.split('    void collisionCheckObjects(', 1)[1].split(
            '    void handleButton(', 1)[0]
        snapshot = collision.index('snapshotObject(object);')
        self.assertIn('typeinfo_cast<RingObject*>(object)', collision[:snapshot])
        self.assertLess(snapshot, collision.index('relevant.push_back(object);'))
        self.assertLess(snapshot, collision.index(
            'GJBaseGameLayer::collisionCheckObjects(player, &relevant'))

    def test_ground_setting_does_not_bypass_touching_orb_timing(self):
        retime = SOURCE.split('        bool orbRetime = ', 1)[1].split(';', 1)[0]
        self.assertIn('touchingOrb', retime)
        self.assertIn('buf.orbTiming', retime)
        self.assertNotIn('buf.orbAlignGround', retime)
        self.assertNotIn('m_isOnGround', retime)

    def test_wave_orb_presses_are_not_excluded(self):
        retime = SOURCE.split('        bool orbRetime = ', 1)[1].split(';', 1)[0]
        self.assertNotIn('m_isDart', retime)
        self.assertIn('if (down && buf.orbAlign && buf.smart)', SOURCE)

    def test_tick_zero_press_has_real_touching_rings(self):
        copy = SOURCE.split('    void copyState(', 1)[1].split(
            '        // Preserve both axes:', 1)[0]
        self.assertIn('addObjectsFromArray(real->m_touchingRings)', copy)
        self.assertIn('snapshotObject(', copy)

    def test_wave_does_not_assume_additional_orb_presses(self):
        wrapper = SOURCE.split('    int simulateOrbs(', 1)[1].split(
            '        buf.touchedOrb = false;', 1)[0]
        self.assertIn('p1->m_isDart', wrapper)
        self.assertLess(wrapper.index('buf.orbClick = false;'),
                        wrapper.index('int ret = simulate('))
        self.assertIn('buf.orbClick = previousOrbClick;', wrapper)

    def test_safe_original_requires_target_activation(self):
        align = SOURCE.split('    int alignOrb(', 1)[1].split(
            '    // v2.1: best delayed run', 1)[0]
        self.assertIn('bool nowUses = activatedByPress(0);', align)
        self.assertIn('ret >= need && activatedByPress(k)', align)
        self.assertNotIn('holdsUp(0, horizon, false)', align)
        self.assertNotIn('leaveSafeInput(shipAlign', align)

    def test_probe_sees_already_touching_orb_without_callback(self):
        sim = SOURCE.split('        // Already-touching rings need not emit', 1)[1]
        seed, rest = sim.split('        buf.held1 = buf.held2', 1)
        self.assertIn('buf.firstOrbTick = 0;', seed)
        self.assertIn('objectAtIndex(0)', seed)
        self.assertIn('for (; tick < total;', rest)

    def test_no_better_timing_still_passes_original_click(self):
        fallback = SOURCE.split('            if (orbRetime || buf.orbWasTiming || (orbTargetFound && !grounded)) {', 1)[1]
        fallback = fallback.split('\n        buf.sawMover', 1)[0]
        self.assertIn('buf.appliedInput = down;', fallback)
        self.assertIn('return false;', fallback)
        self.assertIn('orb hit not confirmed', fallback)


if __name__ == '__main__':
    unittest.main()
