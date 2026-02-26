const PebbleTimelineJs = require("pebble-timeline-js");

const NEXT_TIMER_TIMELINE_PIN_ID = "pebble-timer-plus-next-timer-timeline-pin";

Pebble.addEventListener("ready", function (_) {
  console.log("PebbleKit JS ready!");
});

Pebble.addEventListener("appmessage", function (event) {
  console.log("Received message: " + JSON.stringify(event.payload));

  if (!event.payload || !("type" in event.payload)) return;
  const payload = event.payload;

  console.log("Received message of type: " + payload.type);
  switch (payload.type) {
    case "update_timeline_pin":
      if (Pebble.deleteTimelinePin) {
        Pebble.deleteTimelinePin(NEXT_TIMER_TIMELINE_PIN_ID);
      } else {
        PebbleTimelineJs.deleteUserPin({ id: NEXT_TIMER_TIMELINE_PIN_ID });
      }
      if (payload.update_timeline_pin_time > 0) {
        const nextTimerTime = new Date(payload.update_timeline_pin_time * 1000);
        const timelinePin = {
          id: NEXT_TIMER_TIMELINE_PIN_ID,
          time: nextTimerTime,
          layout: {
            type: "genericPin",
            title: "Timer Done",
            tinyIcon: "system://images/ALARM_CLOCK",
          },
        };
        if (Pebble.insertTimelinePin) {
          Pebble.insertTimelinePin(timelinePin);
        } else {
          PebbleTimelineJs.insertUserPin(timelinePin);
        }
      }
      break;
  }
});
