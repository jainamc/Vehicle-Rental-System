const test = require('node:test');
const assert = require('node:assert/strict');
const { getRecommendedVehicles } = require('./recommendations.js');

test('recommends the best matching vehicles within budget and availability', () => {
  const vehicles = [
    { id: 'HAT-101', model: 'Maruti Swift', type: 'Hatchback', price: 1800, seats: 5, range: 800 },
    { id: 'SUV-201', model: 'Mahindra Thar', type: 'SUV', price: 4200, seats: 4, range: 700 },
    { id: 'EV-401', model: 'Tata Nexon EV', type: 'EV', price: 3200, seats: 5, range: 450 }
  ];

  const bookings = [
    { vehicleId: 'HAT-101', start: '2026-09-28', end: '2026-10-02', returned: false },
    { vehicleId: 'SUV-201', start: '2026-09-30', end: '2026-10-03', returned: false }
  ];

  const recommended = getRecommendedVehicles({
    vehicles,
    bookings,
    startDate: '2026-10-03',
    budget: 6000,
    distance: 300,
    passengers: 2,
    days: 3,
  });

  assert.equal(recommended.length, 1);
  assert.equal(recommended[0].vehicle.id, 'HAT-101');
  assert.ok(recommended[0].score > 0);
  assert.ok(recommended.every((entry) => entry.quote.due <= 6000));
});

test('returns an empty list when no vehicle fits the trip requirements', () => {
  const vehicles = [
    { id: 'SUV-201', model: 'Mahindra Thar', type: 'SUV', price: 4200, seats: 4, range: 700 }
  ];

  const recommended = getRecommendedVehicles({
    vehicles,
    bookings: [],
    startDate: '2026-10-03',
    budget: 2500,
    distance: 500,
    passengers: 5,
    days: 2,
  });

  assert.deepEqual(recommended, []);
});
