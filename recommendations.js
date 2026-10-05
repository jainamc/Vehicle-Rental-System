(function (root) {
  function addDays(iso, days) {
    const date = new Date(iso + 'T00:00:00');
    date.setDate(date.getDate() + Number(days || 0));
    const local = new Date(date.getTime() - date.getTimezoneOffset() * 60000);
    return local.toISOString().slice(0, 10);
  }

  function toTimestamp(value) {
    if (value === null || value === undefined || value === '') return Number.NaN;
    if (value instanceof Date) return value.getTime();
    const raw = String(value).trim();
    if (!raw) return Number.NaN;
    const date = new Date(raw.includes('T') ? raw : `${raw}T00:00:00`);
    return Number.isNaN(date.getTime()) ? Number.NaN : date.getTime();
  }

  function overlaps(startIso, endIso, booking) {
    const candidateStart = toTimestamp(startIso);
    const candidateEnd = toTimestamp(endIso);
    const bookingStart = toTimestamp(booking.startAt || booking.start || startIso);
    const bookingEnd = toTimestamp(booking.endAt || booking.end || endIso);

    if ([candidateStart, candidateEnd, bookingStart, bookingEnd].some((value) => Number.isNaN(value))) {
      return false;
    }

    return candidateStart < bookingEnd && bookingStart < candidateEnd;
  }

  function getRecommendedVehicles({ vehicles = [], bookings = [], startDate, budget, distance, passengers, days = 1, quoteFor }) {
    if (!Array.isArray(vehicles) || !vehicles.length) return [];
    if (!startDate || Number(distance) <= 0 || Number(passengers) <= 0 || Number(budget) <= 0 || Number(days) <= 0) return [];

    const endDate = addDays(startDate, Number(days));
    const candidates = vehicles
      .filter((vehicle) => Number(vehicle?.range || 0) >= Number(distance) && Number(vehicle?.seats || 0) >= Number(passengers))
      .map((vehicle) => {
        const isBooked = bookings.some((booking) => {
          if (!booking || booking.vehicleId !== vehicle.id || booking.returned) return false;
          return overlaps(startDate, endDate, booking);
        });

        if (isBooked) return null;

        const fallbackQuote = { due: Number(vehicle?.price || 0) * Number(days || 1) };
        const quote = typeof quoteFor === 'function' ? quoteFor(vehicle, startDate, Number(days)) : fallbackQuote;
        const due = Number(quote?.due ?? quote?.rental ?? quote?.base ?? 0);
        const normalizedQuote = { ...quote, due: Number.isFinite(due) ? due : 0 };

        return {
          vehicle,
          quote: normalizedQuote,
          score: Math.max(0, (Number(budget) - Number(normalizedQuote.due || 0)) / Number(budget) * 100 + (Number(vehicle.range || 0) - Number(distance)) / 200),
          startDate,
          endDate,
        };
      })
      .filter(Boolean)
      .filter((entry) => Number(entry.quote?.due || Number.MAX_SAFE_INTEGER) <= Number(budget))
      .sort((a, b) => Number(a.quote.due) - Number(b.quote.due) || b.score - a.score);

    return candidates.slice(0, 3);
  }

  const api = { getRecommendedVehicles };
  root.RoadwiseRecommendations = api;
  if (typeof module !== 'undefined' && module.exports) {
    module.exports = api;
  }
})(typeof window !== 'undefined' ? window : globalThis);
