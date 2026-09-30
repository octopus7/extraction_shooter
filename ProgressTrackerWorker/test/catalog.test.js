import test from 'node:test';
import assert from 'node:assert/strict';
import {catalog,validateCatalog} from '../src/catalog.js';
test('catalog has valid acyclic prerequisites',()=>assert.doesNotThrow(()=>validateCatalog(catalog)));
test('catalog rejects duplicates, missing parents and cycles',()=>{
 const entry=(id,requires=[])=>({id,category:'quest',labelKey:'label',requires});
 for(const entries of [[entry('a'),entry('a')],[entry('a',['b'])],[entry('a',['b']),entry('b',['a'])]]){
  assert.throws(()=>validateCatalog(entries));
 }
});
