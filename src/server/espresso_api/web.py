"""Server-rendered, progressively enhanced journal UI (no internal HTTP calls)."""

import json
from datetime import UTC, date, datetime, time
from pathlib import Path
from typing import Annotated, TypeVar
from urllib.parse import urlencode
from uuid import UUID

from fastapi import APIRouter, Depends, HTTPException, Query, Request, Response
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel, ValidationError

from . import store
from .dependencies import DB
from .models import CoffeeCreate, CoffeePatch, SetupWrite, ShotPatch

router = APIRouter(include_in_schema=False)
templates = Jinja2Templates(directory=Path(__file__).with_name("templates"))
T = TypeVar("T", bound=BaseModel)
PAGE_SIZE = 20
Offset = Annotated[int, Query(ge=0)]


def path_query(path: str, /, **values) -> str:
    query = urlencode({key: str(value) for key, value in values.items() if value is not None})
    return path + ("?" + query if query else "")


def display_number(value: float | None) -> str:
    return "—" if value is None else f"{value:g}"


templates.env.globals["path_query"] = path_query
templates.env.filters["number"] = display_number
templates.env.filters["utc_time"] = lambda value: value.astimezone(UTC).strftime(
    "%d %b %Y · %H:%M UTC"
)


def is_web_path(path: str) -> bool:
    return path == "/" or path.split("/")[1] in {"shots", "coffees", "setup"}


def render(request: Request, template: str, status_code: int = 200, **context) -> HTMLResponse:
    return templates.TemplateResponse(
        request=request,
        name=template,
        status_code=status_code,
        context={"errors": {}, "values": {}, **context},
        headers={"Cache-Control": "no-store", "Vary": "HX-Request"},
    )


def error_page(request: Request, status: int, message: str) -> HTMLResponse:
    return render(request, "error.html", status_code=status, status=status, message=message)


def redirect(request: Request, url: str) -> Response:
    if request.headers.get("HX-Request") == "true":
        # Explicit navigation updates both the URL and the boosted page region.
        return Response(
            headers={
                "HX-Location": json.dumps(
                    {
                        "path": url,
                        "target": "#app",
                        "select": "#app",
                        "swap": "outerHTML",
                    }
                )
            }
        )
    return RedirectResponse(url, status_code=303)


async def form_data(request: Request) -> dict[str, str]:
    # Browsers can submit cross-origin forms even on a trusted LAN. Reject those
    # without adding authentication or changing the device-facing JSON API.
    origin = request.headers.get("origin")
    if origin is not None and origin != str(request.base_url).rstrip("/"):
        raise HTTPException(403, "Please submit this form from the journal itself.")
    async with request.form(max_files=0, max_fields=20) as form:
        return {key: str(value) for key, value in form.items()}


FormData = Annotated[dict[str, str], Depends(form_data)]


def form_model(model: type[T], fields: dict[str, str]) -> tuple[T | None, dict[str, str]]:
    """Convert HTML numeric controls, retaining raw input for invalid form renders."""
    values: dict = {}
    errors = {}
    for key, raw in fields.items():
        value = raw.strip()
        if not value and key != "name":
            values[key] = None
        elif key in {"grind_setting", "dose_g", "target_yield_g", "yield_g"}:
            try:
                values[key] = float(value)
            except ValueError:
                errors[key] = "Enter a number."
        elif key == "rating":
            try:
                values[key] = int(value)
            except ValueError:
                errors[key] = "Choose a whole-number rating from 1 to 5."
        else:
            values[key] = value
    if errors:
        return None, errors
    try:
        return model.model_validate(values), {}
    except ValidationError as exc:
        return None, {
            str(error["loc"][0]) if error["loc"] else "form": error["msg"] for error in exc.errors()
        }


def page_links(request: Request, total: int, offset: int) -> dict:
    params = dict(request.query_params)
    return {
        "previous_url": path_query(
            request.url.path, **{**params, "offset": max(0, offset - PAGE_SIZE)}
        )
        if offset
        else None,
        "next_url": path_query(request.url.path, **{**params, "offset": offset + PAGE_SIZE})
        if offset + PAGE_SIZE < total
        else None,
    }


def selected_uuid(value: str) -> UUID | None:
    try:
        return UUID(value) if value else None
    except ValueError as exc:
        raise HTTPException(422, "Choose a valid coffee bag.") from exc


@router.get("/")
def home() -> RedirectResponse:
    return RedirectResponse("/shots", status_code=303)


@router.get("/shots", response_class=HTMLResponse)
def history(
    request: Request,
    db: DB,
    offset: Offset = 0,
    coffee_id: str = "",
    name: Annotated[str | None, Query(max_length=200)] = None,
    roaster: Annotated[str | None, Query(max_length=200)] = None,
    since: Annotated[str, Query(max_length=10)] = "",
    until: Annotated[str, Query(max_length=10)] = "",
) -> HTMLResponse:
    try:
        since_date = date.fromisoformat(since) if since else None
        until_date = date.fromisoformat(until) if until else None
    except ValueError as exc:
        raise HTTPException(422, "Choose valid filter dates.") from exc
    page = store.list_shots(
        db,
        limit=PAGE_SIZE,
        offset=offset,
        coffee_id=selected_uuid(coffee_id),
        name=name,
        roaster=roaster,
        since=datetime.combine(since_date, time.min, UTC) if since_date else None,
        until=datetime.combine(until_date, time.min, UTC) if until_date else None,
    )
    return render(
        request,
        "history.html",
        page=page,
        coffees=store.coffee_choices(db, True),
        setup=store.get_setup(db),
        selected=coffee_id,
        name=name,
        roaster=roaster,
        since=since,
        until=until,
        **page_links(request, page.total, offset),
    )


@router.get("/shots/{shot_id}", response_class=HTMLResponse)
def shot_detail(request: Request, shot_id: UUID, db: DB) -> HTMLResponse:
    shot = store.get_shot(db, shot_id)
    coffee = store.get_coffee(db, shot.setup.coffee.id) if shot.setup.coffee else None
    return render(
        request,
        "shot.html",
        shot=shot,
        coffee=coffee,
        values=shot.model_dump(mode="json"),
        saved=request.query_params.get("saved") == "1",
    )


@router.post("/shots/{shot_id}")
def save_shot(request: Request, shot_id: UUID, fields: FormData, db: DB) -> Response:
    shot = store.get_shot(db, shot_id)
    payload, errors = form_model(ShotPatch, fields)
    if payload is None:
        coffee = store.get_coffee(db, shot.setup.coffee.id) if shot.setup.coffee else None
        return render(
            request,
            "shot.html",
            422,
            shot=shot,
            coffee=coffee,
            values=fields,
            errors=errors,
        )
    store.patch_shot(db, shot_id, payload)
    return redirect(request, f"/shots/{shot_id}?saved=1")


@router.get("/coffees", response_class=HTMLResponse)
def coffees(
    request: Request,
    db: DB,
    offset: Offset = 0,
    include_archived: bool = False,
    q: Annotated[str, Query(max_length=200)] = "",
) -> HTMLResponse:
    page = store.list_coffees(db, PAGE_SIZE, offset, include_archived, q)
    return render(
        request,
        "coffees.html",
        page=page,
        include_archived=include_archived,
        q=q,
        **page_links(request, page.total, offset),
    )


@router.get("/coffees/new", response_class=HTMLResponse)
def new_coffee(request: Request) -> HTMLResponse:
    return render(request, "coffee.html", coffee=None)


@router.post("/coffees/new")
def add_coffee(request: Request, fields: FormData, db: DB) -> Response:
    payload, errors = form_model(CoffeeCreate, fields)
    if payload is None:
        return render(request, "coffee.html", 422, coffee=None, values=fields, errors=errors)
    coffee = store.create_coffee(db, payload)
    return redirect(request, f"/coffees/{coffee.id}?saved=1")


@router.get("/coffees/{coffee_id}", response_class=HTMLResponse)
def coffee_detail(request: Request, coffee_id: UUID, db: DB) -> HTMLResponse:
    coffee = store.get_coffee(db, coffee_id)
    return render(
        request,
        "coffee.html",
        coffee=coffee,
        values=coffee.model_dump(mode="json"),
        saved=request.query_params.get("saved") == "1",
    )


@router.post("/coffees/{coffee_id}")
def save_coffee(request: Request, coffee_id: UUID, fields: FormData, db: DB) -> Response:
    coffee = store.get_coffee(db, coffee_id)
    payload, errors = form_model(CoffeePatch, fields)
    if payload is None:
        return render(request, "coffee.html", 422, coffee=coffee, values=fields, errors=errors)
    store.patch_coffee(db, coffee_id, payload)
    return redirect(request, f"/coffees/{coffee_id}?saved=1")


@router.post("/coffees/{coffee_id}/archive")
def archive_coffee(request: Request, coffee_id: UUID, fields: FormData, db: DB) -> Response:
    if fields.get("archived") not in {"true", "false"} or set(fields) != {"archived"}:
        raise HTTPException(422, "Choose whether to archive or restore this bag.")
    store.patch_coffee(db, coffee_id, CoffeePatch(archived=fields["archived"] == "true"))
    return redirect(request, f"/coffees/{coffee_id}?saved=1")


def setup_context(db: DB) -> dict:
    setup = store.get_setup(db)
    choices = store.coffee_choices(db)
    archived = None
    if setup.coffee and not any(coffee.id == setup.coffee.id for coffee in choices):
        archived = store.get_coffee(db, setup.coffee.id)
        choices.append(archived)
    return {"setup": setup, "coffees": choices, "archived": archived}


@router.get("/setup", response_class=HTMLResponse)
def setup_page(request: Request, db: DB) -> HTMLResponse:
    context = setup_context(db)
    setup = context["setup"]
    values = {
        "coffee_id": str(setup.coffee.id) if setup.coffee else "",
        "grind_setting": setup.grind_setting,
        "dose_g": setup.dose_g,
        "target_yield_g": setup.target_yield_g,
    }
    return render(
        request,
        "setup.html",
        **context,
        values=values,
        saved=request.query_params.get("saved") == "1",
    )


@router.post("/setup")
def save_setup(request: Request, fields: FormData, db: DB) -> Response:
    payload, errors = form_model(SetupWrite, fields)
    status = 422
    if payload is not None:
        try:
            store.set_setup(db, payload)
            return redirect(request, "/setup?saved=1")
        except HTTPException as exc:
            if exc.status_code not in {404, 409}:
                raise
            errors["coffee_id"] = str(exc.detail)
            status = exc.status_code
    return render(request, "setup.html", status, **setup_context(db), values=fields, errors=errors)
