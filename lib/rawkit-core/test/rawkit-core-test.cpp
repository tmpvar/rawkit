#include <doctest.h>

#include <rawkit/core.h>

TEST_CASE("[rawkit/core] resource sources (no deps)") {
  rawkit_resource_t res = {};
  CHECK(rawkit_resource_sources(&res) == false);
}

TEST_CASE("[rawkit/core] resource sources (null dep)") {
  rawkit_resource_t res = {};
  CHECK(rawkit_resource_sources(&res, nullptr, nullptr, nullptr) == false);
  CHECK(rawkit_resource_sources(&res, nullptr, nullptr, nullptr) == false);
}

TEST_CASE("[rawkit/core] resource sources (self reference)") {
  rawkit_resource_t res = {};
  CHECK(rawkit_resource_sources(&res, &res) == false);
}

TEST_CASE("[rawkit/core] resource sources (single dep)") {
  rawkit_resource_t res = {};
  rawkit_resource_t dep = {};
  dep.resource_id = 999;
  dep.resource_version = 0;

  CHECK(rawkit_resource_sources(&res, &dep) == false);
  CHECK(rawkit_resource_sources(&res, &dep) == false);

  dep.resource_version = 1;
  CHECK(rawkit_resource_sources(&res, &dep) == true);
  CHECK(rawkit_resource_sources(&res, &dep) == false);
}

TEST_CASE("[rawkit/core] resource sources (multiple dep)") {
  rawkit_resource_t res = {};
  rawkit_resource_t deps[2] = {};
  deps[0].resource_id = 999;
  deps[0].resource_version = 1;
  deps[1].resource_id = 1;
  deps[1].resource_version = 0;

  CHECK(rawkit_resource_sources(&res, &deps[0], &deps[1]) == true);
  CHECK(rawkit_resource_sources(&res, &deps[0], &deps[1]) == false);

  deps[0].resource_version = 2;
  CHECK(rawkit_resource_sources(&res, &deps[0], &deps[1]) == true);
  CHECK(rawkit_resource_sources(&res, &deps[0], &deps[1]) == false);

  deps[1].resource_version = 1;
  CHECK(rawkit_resource_sources(&res, &deps[0], &deps[1]) == true);
  CHECK(rawkit_resource_sources(&res, &deps[0], &deps[1]) == false);
}
