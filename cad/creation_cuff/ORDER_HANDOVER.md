# Creation Ring: order handover

Everything a Claude session on your own computer needs to order the ring,
plus a printer shortlist. Prices were checked on **2026-11-06**.

Files (in the order pack, or in this repo under `cad/creation_cuff/export/`):

| File | Use |
|---|---|
| `creation_ring_sculpted.3mf` | **Upload this one** (11 MB, millimetres) |
| `creation_ring_sculpted.stl` | Same model, if a site refuses 3MF (45 MB) |
| `preview.jpg`, `metals.jpg` | What it should look like |

**The ring:** Creation Ring, a robot hand and a human hand reaching across
the opening. **US size 11, 20.57 mm inside.** It measures 29.1 × 28.5 × 7.7 mm,
with a volume of 0.66 cm³. It is one watertight solid.

---

## 1. Prompt to paste into the session on your computer

Use the Claude desktop app with **Claude in Chrome** turned on, and attach
or open this folder.

```text
I want to order 3D-printed resin prototypes of my ring from JLC3DP.
Everything is in this folder; ORDER_HANDOVER.md has the details.

1. In Chrome, open https://jlc3dp.com/3d-printing-quote. If I need to sign
   in, ask me to do it myself. Never type a password for me.
2. Upload creation_ring_sculpted.3mf. JLC3DP must read it as
   2.91 x 2.847 x 0.77 cm, volume 0.66 cm³. If it shows anything else,
   stop and tell me.
3. Edit Specifications -> 3D Technology: SLA(Resin). I want three lines,
   quantity 1 each (duplicate the upload if needed):
     - Imagine Black
     - Grey Resin
     - 9600 Resin (White)
   For every line: Surface Finish = No, Thread = No, Package Box = Blank Box.
4. Paste the remark from section 3 of ORDER_HANDOVER.md into each line's
   Remark field.
5. Ship to my Toronto address. Pick the cheapest option marked DDP, which
   means duties are prepaid, so nothing is owed at the door.
6. Stop on the payment page. Show me the itemised total (parts, shipping,
   taxes, any coupon) and wait. Do not pay or submit without my explicit OK.
```

## 2. JLC3DP quote for this ring

USD, quantity 1, before shipping. The quote was read off JLC3DP's instant
quote with this exact file, standard 3-day build.

| Resin | Price | What it is |
|---|---|---|
| **Imagine Black** | $2.13 | "High precision, smooth surface". The best display piece; looks like oxidised metal |
| **Grey Resin** | $0.32 | Shows the detail best, like the clay renders |
| **9600 Resin (white)** | $0.32 | Their default, cost-effective |
| Black Resin / JLC Black Resin | $1.07 | Plain black |
| 8001 Resin | $1.97 | Translucent |
| 8228, LEDO 6060, CBY, X, JLC Temp, 9000HE | $0.32 | Engineering resins; fine but no better for looks |
| WJP Full Color Resin | $6.91 | Inkjet full-colour process |

- Every SLA resin is listed at ±0.2 mm tolerance.
- Shipping is calculated at checkout after you sign in.
- New-account coupons were showing on the page.
- Build: about 3 days, plus 2–5 days shipping.

## 3. Remark to paste on each line

```text
Fine-detail jewellery model (ring, US size 11). Please orient it so both
hands face up, and put supports on the inside of the band only, away from
the fingers and fingertips. The fingers are about 0.9-1.1 mm thick and the
two index fingertips are 0.56 mm apart; please do not sand the hands and keep
that gap open. The thin features are intentional and I accept the risk.
```

**Heads-up**

- JLC3DP's guideline is walls over 1.2 mm and the thinnest part at least
  0.8 mm. The fingers are near that limit, so they may email asking you to
  confirm. Reply that you accept.
- At ±0.2 mm, the 0.14 mm grooves and 0.24 mm beads will soften a little.
  That is expected for a prototype.
- Resin is for looks and checking the fit, not for daily wear: it is brittle
  and yellows in sunlight.

## 4. Afterwards: the silver ring

Pick one of these:

- **i.materialise** (online, ships to Canada):
  https://i.materialise.com/3d-printing-technologies/lost-wax-printing-casting
  Upload the 3MF and choose **Silver** (lost-wax), polished or satin.
- **Toronto casters** (call or email first; details come from directory
  listings):
  - Fine Casting Jewellery, 215 Victoria St
  - Procast / Golden Casting, 21 Dundas Square
  - FTJCO, a jeweller with in-house CAD-to-cast

Quote request you can send to the casters:

```text
Subject: Quote request: sterling silver casting from my 3D file (ring, US 11)

Hi, I have a finished 3D model (3MF/STL, millimetres) of a sculpted ring,
29 x 28 x 8 mm, 0.66 cm³, US size 11. Can you print a castable pattern from
my file and cast it in sterling silver, with a polished finish and a light
oxidised patina in the recesses? It has fine detail (0.14 mm grooves,
0.24 mm beads, fingers about 1 mm). What would it cost and how long would
it take? I can send the file right away.
```

## 5. Printer for robotics parts

Bambu prices were read off Bambu's US store on 2026-11-14, during its Black
Friday sale. Buy from Bambu's Canadian store (ca.store.bambulab.com), which
prices in CAD.

**Recommendation: Bambu Lab H2D with the AMS.**

Robot parts need nylon carbon-fibre, PC and ASA, and those warp unless the
chamber is actively heated. The H2D:

- has a **65 °C heated chamber** and **350 °C nozzles**;
- has **two nozzles**: a soft and a rigid material in one part (e.g. a TPU
  pad on a PETG-CF gripper), or supports that come off cleanly in gearbox
  housings;
- prints **350 × 320 × 325 mm**, enough for arm links and chassis plates in
  one piece;
- takes an optional 10 W/40 W laser and cutting module, for acrylic or
  plywood plates.

| Printer | Price | Verdict |
|---|---|---|
| **Bambu Lab H2D** | US$1,399; AMS Combo US$1,599; Laser Full Combo US$1,949 (normally 1,549 / 1,749 / 2,149) | **The pick.** Two nozzles, heated chamber, large, best ecosystem |
| Bambu Lab H2C | from US$1,899 (normally 2,199) | H2D plus fast automatic nozzle swaps for many materials. Extra cost, little gain for robotics |
| Bambu Lab P2S | US$449; Combo US$649 (normally 549 / 749) | Hardened steel, 300 °C, 50 °C chamber, 256 mm cube. Fine for PETG-CF and ASA; nylon is harder. The budget Bambu |
| Qidi Q2 | about US$449 (pre-sale) | Value pick: 65 °C heated chamber, 370 °C, hardened, 270 × 270 × 256 mm. Single nozzle, smaller ecosystem |
| Qidi Plus4 | about US$699 | Same chamber and temperatures, larger (305 × 305 × 280 mm) |
| Prusa Core One+ | about C$1,760–2,250 (retailers) | Open-source and repairable, but 55 °C chamber and 290 °C |

### Filament for robot parts

Prices are from Bambu's Canadian store unless noted.

| Material | Price per kg | Use |
|---|---|---|
| PETG-CF | from C$40.99 | Everyday brackets and plates: stiff and easy to print |
| PA6-CF / PAHT-CF (nylon carbon-fibre) | about C$55–120 | Gears and parts under load or heat. **Must be dried** |
| ASA | C$38.99 | Housings and outdoor parts (UV-stable) |
| PC | C$50.99 | Tough, heat-resistant parts |
| TPU 95A HF | C$53.99 | Grippers, wheels, bumpers |

- **Drying:** nylon needs about 70–80 °C before printing. Get Bambu's AMS HT
  (sold as an H2D bundle) or a stand-alone dryer such as the Creality Space Pi
  X4 (US$179, up to 85 °C).
- **Bench extras:** brass heat-set inserts (M2–M5) with a soldering-iron tip,
  digital calipers, an M3 screw kit, bearings.
- **Outsource the final parts:** JLC3DP (the ring quote site above) also does
  MJF nylon and metal printing, and its sister service JLCCNC machines
  aluminium. Use those for parts too strong or hot for filament (motor mounts,
  shafts).

Sources:
- https://us.store.bambulab.com/products/h2d
- https://us.store.bambulab.com/products/p2s
- https://ca.store.bambulab.com/en/collections/fiber-reinforced
- https://us.qidi3d.com/collections/3d-printers/products/qidi-q2
- https://www.3djake.com/qidi-tech/plus4
- https://spool3d.ca/original-prusa/
- https://www.bestbuy.com/product/creality-space-pi-x4-dryer-space-grey/CZT3ZRXKWS

### Resin, for jewellery later

| Printer | Price |
|---|---|
| Elegoo Mars 5 Ultra with Mercury Plus wash/cure | C$499 bundle |
| Elegoo Saturn 4 Ultra 16K | C$669 |

Source: https://ca.elegoo.com
