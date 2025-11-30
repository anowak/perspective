// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ ██████ ██████ ██████       █      █      █      █      █ █▄  ▀███ █       ┃
// ┃ ▄▄▄▄▄█ █▄▄▄▄▄ ▄▄▄▄▄█  ▀▀▀▀▀█▀▀▀▀▀ █ ▀▀▀▀▀█ ████████▌▐███ ███▄  ▀█ █ ▀▀▀▀▀ ┃
// ┃ █▀▀▀▀▀ █▀▀▀▀▀ █▀██▀▀ ▄▄▄▄▄ █ ▄▄▄▄▄█ ▄▄▄▄▄█ ████████▌▐███ █████▄   █ ▄▄▄▄▄ ┃
// ┃ █      ██████ █  ▀█▄       █ ██████      █      ███▌▐███ ███████▄ █       ┃
// ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
// ┃ Copyright (c) 2017, the Perspective Authors.                              ┃
// ┃ ╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌ ┃
// ┃ This file is part of the Perspective library, distributed under the terms ┃
// ┃ of the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0). ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

use std::collections::HashSet;
use std::rc::Rc;

use perspective_client::config::*;
use perspective_client::utils::PerspectiveResultExt;
use yew::prelude::*;

use crate::components::containers::select::*;
use crate::components::style::LocalStyle;
use crate::model::*;
use crate::renderer::*;
use crate::session::{AggregateOption, *};
use crate::*;

#[derive(Properties)]
pub struct AggregateSelectorProps {
    pub column: String,
    pub aggregate: Option<Aggregate>,
    pub renderer: Renderer,
    pub session: Session,
}

derive_model!(Renderer, Session for AggregateSelectorProps);

impl PartialEq for AggregateSelectorProps {
    fn eq(&self, rhs: &Self) -> bool {
        self.column == rhs.column && self.aggregate == rhs.aggregate
    }
}

pub enum AggregateSelectorMsg {
    SetAggregate(AggregateOption),
}

pub struct AggregateSelector {
    aggregates: Rc<Vec<SelectItem<AggregateOption>>>,
    aggregate: Option<AggregateOption>,
}

impl Component for AggregateSelector {
    type Message = AggregateSelectorMsg;
    type Properties = AggregateSelectorProps;

    fn create(ctx: &Context<Self>) -> Self {
        let mut selector = Self {
            aggregates: Rc::new(vec![]),
            aggregate: None,
        };

        selector.aggregates = Rc::new(selector.get_dropdown_aggregates(ctx));
        selector
    }

    fn update(&mut self, ctx: &Context<Self>, msg: Self::Message) -> bool {
        match msg {
            AggregateSelectorMsg::SetAggregate(aggregate) => {
                self.set_aggregate(ctx, aggregate);
                false
            },
        }
    }

    fn changed(&mut self, ctx: &Context<Self>, _old: &Self::Properties) -> bool {
        self.aggregates = Rc::new(self.get_dropdown_aggregates(ctx));
        true
    }

    fn view(&self, ctx: &Context<Self>) -> Html {
        let callback = ctx.link().callback(AggregateSelectorMsg::SetAggregate);
        let label_for = |agg: &Aggregate| match agg {
            Aggregate::SingleAggregate(name) => name
                .strip_prefix("udf_reducer_")
                .unwrap_or(name)
                .to_string(),
            Aggregate::MultiAggregate(name, deps) => format!(
                "{} by {}",
                name.strip_prefix("udf_reducer_").unwrap_or(name),
                deps.join(", ")
            ),
        };

        let selected_agg = ctx
            .props()
            .aggregate
            .as_ref()
            .and_then(|agg| {
                self.aggregates
                    .iter()
                    .flat_map(|x| match x {
                        SelectItem::Option(y) => vec![y.clone()],
                        SelectItem::OptGroup(_, y) => y.clone(),
                    })
                    .find(|x| x.aggregate == *agg)
                    .or_else(|| {
                        Some(AggregateOption {
                            aggregate: agg.clone(),
                            display_name: label_for(agg),
                        })
                    })
            })
            .or_else(|| {
                self.aggregates
                    .iter()
                    .flat_map(|x| match x {
                        SelectItem::Option(y) => vec![y.clone()],
                        SelectItem::OptGroup(_, y) => y.clone(),
                    })
                    .next()
            })
            .unwrap_or_else(|| AggregateOption {
                aggregate: Aggregate::SingleAggregate("".to_string()),
                display_name: "".to_string(),
            });

        let values = self.aggregates.clone();
        let label = match &selected_agg.aggregate {
            Aggregate::SingleAggregate(_) => None,
            Aggregate::MultiAggregate(..) => Some(selected_agg.display_name.clone()),
        };

        html! {
            <>
                <LocalStyle href={css!("aggregate-selector")} />
                <div class="aggregate-selector-wrapper">
                    <Select<AggregateOption>
                        wrapper_class="aggregate-selector"
                        {values}
                        label={label.map(|x| x.into())}
                        selected={selected_agg}
                        on_select={callback}
                    />
                </div>
            </>
        }
    }
}

impl AggregateSelector {
    pub fn set_aggregate(&mut self, ctx: &Context<Self>, aggregate: AggregateOption) {
        self.aggregate = Some(aggregate.clone());
        let mut aggregates = ctx.props().session.get_view_config().aggregates.clone();
        aggregates.insert(ctx.props().column.clone(), aggregate.aggregate);
        let config = ViewConfigUpdate {
            aggregates: Some(aggregates),
            ..ViewConfigUpdate::default()
        };

        ctx.props()
            .update_and_render(config)
            .map(ApiFuture::spawn)
            .unwrap_or_log();
    }

    pub fn get_dropdown_aggregates(&self, ctx: &Context<Self>) -> Vec<SelectItem<AggregateOption>> {
        let aggregates = ctx
            .props()
            .session
            .metadata()
            .get_column_aggregate_options(&ctx.props().column)
            .map(|x| x.collect::<Vec<_>>())
            .unwrap_or_default();

        let multi_aggregates2 = aggregates
            .clone()
            .into_iter()
            .flat_map(|x| match x {
                AggregateOption {
                    aggregate: Aggregate::MultiAggregate(x, _),
                    ..
                } => Some(x),
                _ => None,
            })
            .collect::<HashSet<_>>()
            .into_iter()
            .map(|x| {
                SelectItem::OptGroup(
                    x.clone().into(),
                    aggregates
                        .iter()
                        .filter(|y| {
                            matches!(
                                y.aggregate,
                                Aggregate::MultiAggregate(ref z, _)
                                    if &x == z
                            )
                        })
                        .cloned()
                        .collect(),
                )
            })
            .collect::<Vec<_>>();

        let s = aggregates
            .iter()
            .filter(|x| matches!(x.aggregate, Aggregate::SingleAggregate(_)))
            .cloned()
            .map(SelectItem::Option)
            .chain(multi_aggregates2);

        s.collect::<Vec<_>>()
    }
}
