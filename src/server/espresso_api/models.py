"""API contracts. All measurements include their units in the field name."""

from datetime import UTC, date, datetime
from typing import Annotated
from uuid import UUID

from pydantic import (
    AwareDatetime,
    BaseModel,
    ConfigDict,
    Field,
    field_validator,
    model_validator,
)

NonNegative = Annotated[float, Field(ge=0, strict=True)]
Positive = Annotated[float, Field(gt=0, strict=True)]
Temperature = Annotated[float, Field(strict=True)]
Name = Annotated[str, Field(min_length=1, max_length=200)]
Notes = Annotated[str, Field(max_length=4000)]
Rating = Annotated[int, Field(ge=1, le=5, strict=True)]
Revision = Annotated[int, Field(ge=1, strict=True)]


class Model(BaseModel):
    model_config = ConfigDict(extra="forbid", allow_inf_nan=False, str_strip_whitespace=True)


class CoffeeCreate(Model):
    name: Name
    roaster: Name | None = None
    origin: Name | None = None
    roast_date: date | None = None
    notes: Notes | None = None


class CoffeePatch(Model):
    name: Name | None = None
    roaster: Name | None = None
    origin: Name | None = None
    roast_date: date | None = None
    notes: Notes | None = None
    archived: bool | None = None

    @model_validator(mode="after")
    def reject_null_for_required_fields(self):
        # Omitted fields are unchanged, but these two fields cannot be cleared.
        for name in self.model_fields_set & {"name", "archived"}:
            if getattr(self, name) is None:
                raise ValueError(f"{name} cannot be null")
        return self


class Coffee(CoffeeCreate):
    id: UUID
    archived: bool
    created_at: AwareDatetime
    updated_at: AwareDatetime


class CoffeeSnapshot(Model):
    id: UUID
    name: Name
    roaster: Name | None = None
    origin: Name | None = None
    roast_date: date | None = None


class SetupWrite(Model):
    coffee_id: UUID | None = None
    grind_setting: NonNegative | None = None
    dose_g: Positive | None = None
    target_yield_g: Positive | None = None


class Setup(Model):
    revision: Revision
    coffee: CoffeeSnapshot | None = None
    grind_setting: NonNegative | None = None
    dose_g: Positive | None = None
    target_yield_g: Positive | None = None
    updated_at: AwareDatetime


class ShotCreate(Model):
    id: UUID
    setup_revision: Revision
    started_at: AwareDatetime | None = None
    duration_s: NonNegative = Field(
        description="Actual main-brew seconds, including HOLD; excludes preinfusion and bloom."
    )
    configured_preinfusion_s: NonNegative | None = None
    configured_bloom_s: NonNegative | None = None
    configured_brew_s: NonNegative | None = None
    temperature_setpoint_c: Temperature | None = None
    end_temperature_c: Temperature | None = None
    dose_g: Positive | None = None
    yield_g: Positive | None = None
    rating: Rating | None = None
    notes: Notes | None = None

    @field_validator("started_at")
    @classmethod
    def normalize_timestamp(cls, value: datetime | None) -> datetime | None:
        return value.astimezone(UTC) if value else None


class ShotPatch(Model):
    # Brewing measurements and the setup snapshot cannot be rewritten.
    dose_g: Positive | None = None
    yield_g: Positive | None = None
    rating: Rating | None = None
    notes: Notes | None = None


class Shot(Model):
    id: UUID
    setup: Setup
    started_at: AwareDatetime | None
    received_at: AwareDatetime
    duration_s: NonNegative = Field(
        description="Actual main-brew seconds, including HOLD; excludes preinfusion and bloom."
    )
    configured_preinfusion_s: NonNegative | None
    configured_bloom_s: NonNegative | None
    configured_brew_s: NonNegative | None
    temperature_setpoint_c: Temperature | None
    end_temperature_c: Temperature | None
    dose_g: Positive | None
    yield_g: Positive | None
    rating: Rating | None
    notes: Notes | None


class CoffeePage(Model):
    items: list[Coffee]
    total: int
    limit: int
    offset: int


class ShotPage(Model):
    items: list[Shot]
    total: int
    limit: int
    offset: int
