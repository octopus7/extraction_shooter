PRAGMA foreign_keys = ON;

DELETE FROM workspaces
WHERE catalog_id IN (
  SELECT id
  FROM quest_catalogs
  WHERE id = 'catalog-runtime-snapshot'
     OR slug = 'runtime-snapshot'
);

DELETE FROM quest_catalogs
WHERE id = 'catalog-runtime-snapshot'
   OR slug = 'runtime-snapshot';
