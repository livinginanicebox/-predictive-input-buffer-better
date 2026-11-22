#include "../src/prediction_policy.hpp"
#include <cassert>
#include <vector>

struct Orb { int left, right; };
struct Contacts {
    std::vector<Orb*> objects;
    unsigned count() const { return static_cast<unsigned>(objects.size()); }
    Orb* objectAtIndex(unsigned i) const { return objects.at(i); }
    void removeObjectAtIndex(unsigned i) { objects.erase(objects.begin() + i); }
};

int main() {
    using prediction_policy::pruneOrbContacts;
    Orb target{0, 12}, next{30, 40};
    Contacts contacts{{&target}};
    int playerX = 0;
    auto overlaps = [&](Orb* orb) { return playerX >= orb->left && playerX <= orb->right; };
    // Reproduce the reported +48t pick: a copied contact persists even though
    // the unpressed player has moved past the orb. It must not be clickable then.
    for (; playerX <= 48; ++playerX) {
        pruneOrbContacts(&contacts, overlaps);
        assert((contacts.count() > 0) == (playerX <= 12));
    }
    assert(contacts.count() == 0);

    // Only the next orb is in reach; it must not extend the target's touch window.
    playerX = 35;
    contacts.objects = {&target, &next, &target};
    pruneOrbContacts(&contacts, overlaps);
    assert(contacts.count() == 1 && contacts.objectAtIndex(0) == &next);
    playerX = 50;
    pruneOrbContacts(&contacts, overlaps);
    assert(contacts.count() == 0);
    pruneOrbContacts(&contacts, overlaps); // empty list
    pruneOrbContacts(static_cast<Contacts*>(nullptr), overlaps);

    // Orb movement also expires a contact, even when the player did not move.
    playerX = 5;
    contacts.objects = {&target};
    target = {70, 82};
    pruneOrbContacts(&contacts, overlaps);
    assert(contacts.count() == 0);
}
